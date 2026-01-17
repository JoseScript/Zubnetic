/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
XYscopeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gainDb", "Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "zoom", "Zoom",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.001f, 0.5f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "rotateDeg", "Rotate",
        juce::NormalisableRange<float>(-180.0f, 180.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "persistence", "Tracer",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.85f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "saturation", "Saturate",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hueShift", "Hue",
        juce::NormalisableRange<float>(-0.5f, 0.5f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "monoWraps", "Mono Wraps",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.1f), 3.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "monoAmount", "Mono Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "thickness", "Thick",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "monoShape", "Mono Shape",
        juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "waveType", "Wave Type",
        juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "glowIntensity", "Glow Intensity",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "glowSize", "Glow Size",
        juce::NormalisableRange<float>(1.0f, 15.0f, 0.1f), 5.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "particleMode", "Wave/Particle",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fftMode", "FFT Color Mode",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "dcOffset", "R.E.M.",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "invertColors", "Invert",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

    return { params.begin(), params.end() };
}

// constructor
XYscopeAudioProcessor::XYscopeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
#else
    : apvts(*this, nullptr, "PARAMS", createParameterLayout())
#endif
{
    gainDbParam = apvts.getRawParameterValue("gainDb");
    zoomParam = apvts.getRawParameterValue("zoom");
    rotateDegParam = apvts.getRawParameterValue("rotateDeg");
    persistParam = apvts.getRawParameterValue("persistence");
    saturationParam = apvts.getRawParameterValue("saturation");
    hueShiftParam = apvts.getRawParameterValue("hueShift");
    monoWrapsParam = apvts.getRawParameterValue("monoWraps");
    monoAmountParam = apvts.getRawParameterValue("monoAmount");
    thicknessParam = apvts.getRawParameterValue("thickness");
    monoShapeParam = apvts.getRawParameterValue("monoShape");
    waveTypeParam = apvts.getRawParameterValue("waveType");
    glowIntensityParam = apvts.getRawParameterValue("glowIntensity");  
    glowSizeParam = apvts.getRawParameterValue("glowSize");
    particleModeParam = apvts.getRawParameterValue("particleMode");
    fftModeParam = apvts.getRawParameterValue("fftMode");
    dcOffsetParam = apvts.getRawParameterValue("dcOffset");         
    invertColorsParam = apvts.getRawParameterValue("invertColors");

    ringL.resize(ringSize);
    ringR.resize(ringSize);
}

XYscopeAudioProcessor::~XYscopeAudioProcessor()
{
}

//==============================================================================
const juce::String XYscopeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool XYscopeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool XYscopeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool XYscopeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double XYscopeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int XYscopeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int XYscopeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void XYscopeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String XYscopeAudioProcessor::getProgramName (int index)
{
    return {};
}

void XYscopeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void XYscopeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void XYscopeAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool XYscopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void XYscopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    auto numSamples = buffer.getNumSamples();
    auto* left = buffer.getReadPointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : left;

    // Push raw samples for visualization; apply gain/zoom in the editor.
    pushSamples(left, right, numSamples);

    // ADD FFT ANALYSIS:
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = (left[i] + right[i]) * 0.5f;

        if (fftPos < fftSize)
        {
            fftData[fftPos] = mono;
            fftData[fftPos + fftSize] = 0.0f; // imaginary part
            ++fftPos;
        }

        if (fftPos >= fftSize)
        {
            // Perform FFT
            fft.performFrequencyOnlyForwardTransform(fftData.data());

            // Analyze frequency bands
            float bass = 0.0f, mid = 0.0f, high = 0.0f;

            // Bass: 20-250 Hz (bins 0-12 at 44.1kHz)
            for (int bin = 0; bin < 13; ++bin)
                bass += fftData[bin];
            bass /= 13.0f;

            // Mids: 250-2000 Hz (bins 13-100)
            for (int bin = 13; bin < 100; ++bin)
                mid += fftData[bin];
            mid /= 87.0f;

            // Highs: 2000+ Hz (bins 100-512)
            for (int bin = 100; bin < fftSize / 2; ++bin)
                high += fftData[bin];
            high /= (fftSize / 2 - 100);

            // Store normalized values
            bassEnergy.store(juce::jlimit(0.0f, 1.0f, bass * 0.1f));
            midEnergy.store(juce::jlimit(0.0f, 1.0f, mid * 0.1f));
            highEnergy.store(juce::jlimit(0.0f, 1.0f, high * 0.1f));

            fftPos = 0;
        }
    }

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool XYscopeAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* XYscopeAudioProcessor::createEditor()
{
    return new XYscopeAudioProcessorEditor (*this);
}

//==============================================================================
void XYscopeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void XYscopeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr)
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XYscopeAudioProcessor();
}

//==============================================================================
void XYscopeAudioProcessor::pushSamples(const float* left, const float* right, int numSamples)
{
    int start1, size1, start2, size2;
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
    {
        std::memcpy(ringL.data() + start1, left, (size_t)size1 * sizeof(float));
        std::memcpy(ringR.data() + start1, right, (size_t)size1 * sizeof(float));
    }

    if (size2 > 0)
    {
        std::memcpy(ringL.data() + start2, left + size1, (size_t)size2 * sizeof(float));
        std::memcpy(ringR.data() + start2, right + size1, (size_t)size2 * sizeof(float));
    }

    fifo.finishedWrite(size1 + size2);
}

int XYscopeAudioProcessor::pullSamples(float* destL, float* destR, int maxSamples)
{
    int start1, size1, start2, size2;
    fifo.prepareToRead(maxSamples, start1, size1, start2, size2);

    if (size1 > 0)
    {
        std::memcpy(destL, ringL.data() + start1, (size_t)size1 * sizeof(float));
        std::memcpy(destR, ringR.data() + start1, (size_t)size1 * sizeof(float));
    }

    if (size2 > 0)
    {
        std::memcpy(destL + size1, ringL.data() + start2, (size_t)size2 * sizeof(float));
        std::memcpy(destR + size1, ringR.data() + start2, (size_t)size2 * sizeof(float));
    }

    fifo.finishedRead(size1 + size2);
    return size1 + size2;
}
