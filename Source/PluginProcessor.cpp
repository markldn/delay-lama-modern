#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
constexpr float formants[5][3] = {{280,600,2240},{450,800,2830},{800,1150,2900},{350,2000,2800},{270,2140,2950}};
float smooth(float a, float b, float t) { t = juce::jlimit(0.0f, 1.0f, t); return a + (b-a)*t*t*(3.0f-2.0f*t); }
}

juce::AudioProcessorValueTreeState::ParameterLayout DelayLamaProcessor::createParameters() {
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<P>("pitch", "Pitch", juce::NormalisableRange<float>(36, 84, 0.01f), 60));
    p.add(std::make_unique<P>("vowel", "Vowel", 0.0f, 1.0f, 0.5f));
    p.add(std::make_unique<P>("glide", "Glide", 0.0f, 1.0f, 0.15f));
    p.add(std::make_unique<P>("delay", "Delay", 0.0f, 1.0f, 0.18f));
    p.add(std::make_unique<P>("head", "Head Size", 0.0f, 1.0f, 0.5f));
    p.add(std::make_unique<P>("level", "Level", 0.0f, 1.0f, 0.75f));
    p.add(std::make_unique<P>("input", "Mic / Audio Input", 0.0f, 1.0f, 0.0f));
    return p;
}

DelayLamaProcessor::DelayLamaProcessor() : AudioProcessor(BusesProperties().withInput("Audio Input", juce::AudioChannelSet::mono(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)), state(*this, nullptr, "PARAMETERS", createParameters()) {}
bool DelayLamaProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto in=l.getMainInputChannelSet(), out=l.getMainOutputChannelSet();
    return (in.isDisabled() || in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}
void DelayLamaProcessor::prepareToPlay(double sr, int) { sampleRate = sr; phase=0; delayWrite=0; std::fill(std::begin(delayL),std::end(delayL),0.0f); std::fill(std::begin(delayR),std::end(delayR),0.0f); std::memset(inX1,0,sizeof(inX1)); std::memset(inX2,0,sizeof(inX2)); std::memset(inY1,0,sizeof(inY1)); std::memset(inY2,0,sizeof(inY2)); }
void DelayLamaProcessor::noteOn(int note, float vel) { targetHz = (float)juce::MidiMessage::getMidiNoteInHertz(note); velocity=vel; envelope=1.0f; noteHeld=true; }

void DelayLamaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n=buffer.getNumSamples();
    auto input=getBusBuffer(buffer,true,0); auto output=getBusBuffer(buffer,false,0);
    const int inputChannels=input.getNumChannels(), outputChannels=output.getNumChannels();
    const float* inputL=inputChannels>0?input.getReadPointer(0):nullptr; const float* inputR=inputChannels>1?input.getReadPointer(1):inputL;
    auto* outL=output.getWritePointer(0); auto* outR=outputChannels>1?output.getWritePointer(1):outL;
    const auto* pitchP=state.getRawParameterValue("pitch");
    const auto* levelP=state.getRawParameterValue("level");
    const float vp=state.getRawParameterValue("vowel")->load(), gp=state.getRawParameterValue("glide")->load(), dp=state.getRawParameterValue("delay")->load(), hp=state.getRawParameterValue("head")->load();
    if (vp != lastVowelParam) vowelFromCc=false; if (gp != lastGlideParam) glideFromCc=false; if (dp != lastDelayParam) delayFromCc=false; if (hp != lastHeadParam) headFromCc=false;
    lastVowelParam=vp; lastGlideParam=gp; lastDelayParam=dp; lastHeadParam=hp;
    vowel=vowelFromCc?ccVowel:vp; glide=glideFromCc?ccGlide:gp; delayMix=delayFromCc?ccDelay:dp; headSize=headFromCc?ccHead:hp;
    const float level=levelP->load()*volumeCc, inputLevel=state.getRawParameterValue("input")->load();
    const int requestedUiNote=uiNote.load();
    if(requestedUiNote != activeUiNote) { if(requestedUiNote>=0) noteOn(requestedUiNote,0.85f); else if(activeUiNote>=0) { velocity=0.0f; noteHeld=false; } activeUiNote=requestedUiNote; }
    for (const auto m : midi) { const auto msg=m.getMessage(); if(msg.isNoteOn()) noteOn(msg.getNoteNumber(),msg.getFloatVelocity()); else if(msg.isNoteOff()) { velocity=0.0f; noteHeld=false; } else if(msg.isPitchWheel()) { vowel=juce::jlimit(0.0f,1.0f,msg.getPitchWheelValue()/16383.0f); ccVowel=vowel; vowelFromCc=true; } else if(msg.isController()) { if(msg.getControllerNumber()==5) { ccGlide=glide=msg.getControllerValue()/127.0f; glideFromCc=true; } if(msg.getControllerNumber()==12) { ccDelay=delayMix=msg.getControllerValue()/127.0f; delayFromCc=true; } if(msg.getControllerNumber()==13) { ccHead=headSize=msg.getControllerValue()/127.0f; headFromCc=true; } if(msg.getControllerNumber()==7) volumeCc=msg.getControllerValue()/127.0f; if(msg.getControllerNumber()==1) vibrato=msg.getControllerValue()/127.0f; } }
    if (!noteHeld) targetHz=(float)juce::MidiMessage::getMidiNoteInHertz((int)std::round(pitchP->load()));
    const float nyquist=(float)(sampleRate*0.45);
    const float v=vowel*4.0f; const int row=juce::jmin(3,(int)v); const float f=v-row;
    for(int k=0;k<3;++k) formantHz[k]=juce::jlimit(80.0f,nyquist,smooth(formants[row][k],formants[row+1][k],f)*(0.72f+headSize*0.56f));
    float b0[3], b2[3], a1[3], a2[3];
    for(int k=0;k<3;++k) { const float w=juce::MathConstants<float>::twoPi*formantHz[k]/(float)sampleRate, q=7.0f+k*2.0f, alpha=std::sin(w)/(2.0f*q), a0=1.0f+alpha; b0[k]=alpha/a0; b2[k]=-alpha/a0; a1[k]=-2.0f*std::cos(w)/a0; a2[k]=(1.0f-alpha)/a0; }
    for(int i=0;i<n;++i) {
        const float glideTime=0.002f+glide*0.45f;
        pitchHz += (targetHz-pitchHz) * (1.0f-std::exp(-1.0f/(float)(sampleRate*glideTime)));
        const float vibratoHz=pitchHz*(1.0f+0.012f*vibrato*std::sin(lfoPhase)); lfoPhase=std::fmod(lfoPhase+juce::MathConstants<double>::twoPi*5.0/sampleRate,juce::MathConstants<double>::twoPi);
        phase += vibratoHz/sampleRate; bool pulse=false; if(phase>=1.0){phase-=1.0;pulse=true;}
        if(pulse) for(float& a:age) a=0.0f;
        float voice=0.0f;
        for(int k=0;k<3;++k) if(age[k]<0.085f) { const float t=age[k]; const float bw=35.0f+k*22.0f; const float env=std::exp(-t*bw*juce::MathConstants<float>::twoPi); voice += std::sin(juce::MathConstants<float>::twoPi*formantHz[k]*t)*env*(k==0?0.65f:k==1?0.42f:0.28f); age[k]+=1.0f/(float)sampleRate; }
        envelope += ((velocity>0.0f?velocity:0.0f)-envelope) * (velocity>0.0f ? 0.0008f : 0.00012f);
        float mic[2]{};
        const float inSample[2]={inputL?inputL[i]:0.0f,inputR?inputR[i]:0.0f};
        for(int ch=0;ch<juce::jmin(2,outputChannels);++ch) { const int srcCh=(inputChannels>1?ch:0); const float x=inputChannels>0?(srcCh==0?inSample[0]:inSample[1]):0.0f; for(int k=0;k<3;++k) { const float y=b0[k]*x+b2[k]*inX2[ch][k]-a1[k]*inY1[ch][k]-a2[k]*inY2[ch][k]; inX2[ch][k]=inX1[ch][k]; inX1[ch][k]=x; inY2[ch][k]=inY1[ch][k]; inY1[ch][k]=y; mic[ch]+=y*(k==0?0.50f:k==1?0.32f:0.24f); } }
        const float synth=voice*envelope*level*2.4f;
        const float dryL=std::tanh(synth+mic[0]*inputLevel*2.5f), dryR=std::tanh(synth+mic[1]*inputLevel*2.5f);
        const int tap=(int)(sampleRate*(0.19+0.16*headSize)); const int read=(delayWrite+192000-tap)%192000;
        const float dl=delayL[read], dr=delayR[read]; delayL[delayWrite]=dryL+dr*0.38f; delayR[delayWrite]=dryR+dl*0.38f; delayWrite=(delayWrite+1)%192000;
        outL[i]=dryL*(1.0f-delayMix)+dl*delayMix; if(outR!=outL) outR[i]=dryR*(1.0f-delayMix)+dr*delayMix;
    }
}
juce::AudioProcessorEditor* DelayLamaProcessor::createEditor(){ return new DelayLamaEditor(*this); }
void DelayLamaProcessor::getStateInformation(juce::MemoryBlock& d){ auto x=state.copyState(); std::unique_ptr<juce::XmlElement> xml(x.createXml()); copyXmlToBinary(*xml,d); }
void DelayLamaProcessor::setStateInformation(const void* d,int s){ std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d,s)); if(xml&&xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){ return new DelayLamaProcessor(); }
