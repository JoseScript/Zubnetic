#pragma once

#include <JuceHeader.h>

class XYscopeAudioProcessor; // forward declare

class XYscopeAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit XYscopeAudioProcessorEditor(XYscopeAudioProcessor&);
    ~XYscopeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    float visualGainSmoothed = 1.0f;
    float colourEnergySmoothed = 0.0f;
    float hueAccumulator = 0.0f;
    float dcPhase = 0.0f;

private:
    void timerCallback() override;
    void renderFrame();

    XYscopeAudioProcessor& processor;

    juce::Image accumulation;
    std::vector<float> scratchL, scratchR;
    std::vector<juce::Point<float>> points;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYscopeAudioProcessorEditor)
};
