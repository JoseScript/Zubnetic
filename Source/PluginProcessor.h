#pragma once

#include <JuceHeader.h>
#include <array>

//==============================================================================
class XYscopeAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    XYscopeAudioProcessor();
    ~XYscopeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // PARAMETERS
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float>* gainDbParam = nullptr;
    std::atomic<float>* zoomParam = nullptr;
    std::atomic<float>* rotateDegParam = nullptr;
    std::atomic<float>* persistParam = nullptr;
    std::atomic<float>* saturationParam = nullptr;  
    std::atomic<float>* hueShiftParam = nullptr;   
    std::atomic<float>* monoWrapsParam = nullptr; 
    std::atomic<float>* monoAmountParam = nullptr;  
    std::atomic<float>* thicknessParam = nullptr;
    std::atomic<float>* monoShapeParam = nullptr;
    std::atomic<float>* waveTypeParam = nullptr;
    std::atomic<float>* glowIntensityParam = nullptr;  
    std::atomic<float>* glowSizeParam = nullptr;
    std::atomic<float>* particleModeParam = nullptr;
    std::atomic<float>* fftModeParam = nullptr;
    std::atomic<float>* dcOffsetParam = nullptr;    
    std::atomic<float>* invertColorsParam = nullptr;       

    // ---- Scope FIFO (audio thread -> UI thread) ----
    static constexpr int ringSize = 1 << 17; // 131072 samples
    juce::AbstractFifo fifo{ ringSize };
    std::vector<float> ringL, ringR;

    void pushSamples(const float* left, const float* right, int numSamples);
    int  pullSamples(float* destL, float* destR, int maxSamples);
    std::atomic<float> bassEnergy{ 0.0f };
    std::atomic<float> midEnergy{ 0.0f };
    std::atomic<float> highEnergy{ 0.0f };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYscopeAudioProcessor)

        // ADD THESE FFT members:
        static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder; // 1024
    juce::dsp::FFT fft{ fftOrder };
    std::array<float, fftSize * 2> fftData;
    int fftPos = 0;
};
