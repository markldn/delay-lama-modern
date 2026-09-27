#pragma once
#include "PluginProcessor.h"

class DelayLamaEditor final : public juce::AudioProcessorEditor, private juce::Timer, private juce::KeyListener {
public:
    explicit DelayLamaEditor(DelayLamaProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void focusLost(FocusChangeType) override;
    void visibilityChanged() override;
private:
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;
    bool keyStateChanged(bool, juce::Component*) override;
    bool keyPressed(const juce::KeyPress&) override;
    bool keyStateChanged(bool) override;
    bool handleComputerKeyPress(const juce::KeyPress&);
    bool handleComputerKeyState(bool);
    void updateUiNote();
    DelayLamaProcessor& processor;
    juce::Image faceSheet, background;
    juce::Slider glide, delay, head, pitch, input;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttachment, delayAttachment, headAttachment, pitchAttachment, inputAttachment;
    juce::Label title, hint;
    juce::Rectangle<int> pad;
    juce::Rectangle<int> keyboard;
    int pressedKey = -1;
    int computerNote = -1, sentUiNote = -1;
    bool computerKeysDown[13]{};
    int keyAt(juce::Point<int>) const;
    void updatePressedKey(juce::Point<int>);
};
