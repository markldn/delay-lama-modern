#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

class DelayLamaProcessor final : public juce::AudioProcessor {
public:
    DelayLamaProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void setUiNote(int note, bool down) { uiNote.store(down ? note : -1); }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Delay Lama Modern"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
private:
    void noteOn(int note, float velocity);
    double sampleRate = 44100.0;
    double phase = 0.0;
    double pitchHz = 220.0;
    float targetHz = 220.0f, velocity = 0.0f, envelope = 0.0f;
    bool noteHeld = false;
    float vowel = 0.5f, headSize = 0.5f, glide = 0.0f, delayMix = 0.0f;
    float ccVowel = 0.5f, ccGlide = 0.0f, ccDelay = 0.0f, ccHead = 0.5f;
    float volumeCc = 1.0f, vibrato = 0.0f, lfoPhase = 0.0f;
    float lastVowelParam = 0.5f, lastGlideParam = 0.15f, lastDelayParam = 0.18f, lastHeadParam = 0.5f;
    bool vowelFromCc = false, glideFromCc = false, delayFromCc = false, headFromCc = false;
    std::atomic<int> uiNote{-1};
    int activeUiNote = -1;
    float inX1[2][3]{}, inX2[2][3]{}, inY1[2][3]{}, inY2[2][3]{};
    float age[3]{};
    float formantHz[3]{450, 1150, 2830};
    float delayL[192000]{}, delayR[192000]{};
    int delayWrite = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayLamaProcessor)
};
