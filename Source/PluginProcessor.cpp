/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fstream>  

//==============================================================================
LoomAudioProcessor::LoomAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withInput("AuxInput", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
}

LoomAudioProcessor::~LoomAudioProcessor()
{

}

//==============================================================================
const juce::String LoomAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LoomAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LoomAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LoomAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LoomAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LoomAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LoomAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LoomAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LoomAudioProcessor::getProgramName (int index)
{
    return {};
}

void LoomAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void LoomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec{};
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    //auto layout = getBusesLayout();
    //layout.getChannelSet(true, 1) = juce::AudioChannelSet::stereo();  // Set Aux Input Bus
    //setBusesLayout(layout);  // Apply the layout

    setLatencySamples(fft[0].getLatencyInSamples());

    fft[0].reset();
    fft[1].reset();

}

void LoomAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LoomAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Check if the number of input and output buses match the expected count
    if (layouts.inputBuses.size() != 2 || layouts.outputBuses.size() != 1)
        return false; // Expect 2 input buses and 1 output bus

    // Check that each input bus is stereo
    auto mainInput = layouts.getChannelSet(true, 0);
    auto auxInput = layouts.getChannelSet(true, 1);
    auto output = layouts.getChannelSet(false, 0);

    // Ensure all buses are stereo
    if (mainInput == juce::AudioChannelSet::stereo() &&
        auxInput == juce::AudioChannelSet::stereo() &&
        output == juce::AudioChannelSet::stereo())
    {
        return true;
    }

    return false;

}
#endif

void LoomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numInputChannels = getTotalNumInputChannels();
    auto numOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    for (auto i = numInputChannels; i < numOutputChannels; ++i) {
        buffer.clear(i, 0, numSamples);
    }

    float* channelL = buffer.getWritePointer(0);
    float* channelR = buffer.getWritePointer(1);
    float* channelLA = buffer.getWritePointer(2);
    float* channelRA = buffer.getWritePointer(3);

    bool bypass = 0;

    auto chainSettings = getChainSettings(apvts);


    // Processing on a sample-by-sample basis:
    for (int sample = 0; sample < numSamples; ++sample) {
        float sampleL = channelL[sample];
        float sampleR = channelR[sample];
        float sampleLA = channelLA[sample];
        float sampleRA = channelRA[sample];

        sampleL = fft[0].processSample(sampleL, sampleLA, chainSettings);
        sampleR = fft[1].processSample(sampleR, sampleRA, chainSettings);

        channelL[sample] = sampleL;
        channelR[sample] = sampleR;
    }
    
}

//==============================================================================
bool LoomAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LoomAudioProcessor::createEditor()
{
    //return new LoomAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void LoomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

}

void LoomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    
}

juce::AudioProcessorValueTreeState::ParameterLayout
LoomAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("bypassed", "Bypass", juce::NormalisableRange <float>(0.f, 1.f, 1.f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("morphFactor", "Morph Factor", juce::NormalisableRange <float>(0.f,1.f,0.02f, 1.f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("formantShiftFactor", "Formant", juce::NormalisableRange <float>(0.f, 2.f, 0.05f, 1.f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("magProcessing", "Magnitude Processing", juce::NormalisableRange <float>(0.f, 5.f, 1.f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("phaseProcessing", "Phase Processing", juce::NormalisableRange <float>(0.f, 5.f, 1.f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("invertPhase", "Invert Phase", juce::NormalisableRange <float>(0.f, 1.f, 1.f, 1.f), 0.f));
    

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoomAudioProcessor();
}

void LoomAudioProcessor::generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude, double sampleRate)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Fill the buffer with a sine wave on all channels
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Calculate the time `t` in seconds for the current sample
            float t = static_cast<float>(sample) / sampleRate;

            // Generate the sine wave for this sample
            channelData[sample] = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * t);
        }
    }
}

void LoomAudioProcessor::outputToCSV(float* data, int numSamples, const std::string& fileName)
{
    // Open a file in write mode
    std::ofstream file(fileName);

    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    // Write data to the CSV file
    for (size_t i = 0; i < numSamples; ++i) {
        file << data[i];
        if (i < numSamples - 1) {
            file << ",";  // Separate values by commas
        }
    }

    // Close the file
    file.close();
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.bypassed = apvts.getRawParameterValue("bypassed")->load(); // Non-normalized parameters
    settings.morphFactor = apvts.getRawParameterValue("morphFactor")->load(); // Non-normalized parameters
    settings.formantShiftFactor = apvts.getRawParameterValue("formantShiftFactor")->load(); // Non-normalized parameters
    settings.magProcessing = apvts.getRawParameterValue("magProcessing")->load(); // Non-normalized parameters
    settings.phaseProcessing = apvts.getRawParameterValue("phaseProcessing")->load(); // Non-normalized parameters
    settings.invertPhase = apvts.getRawParameterValue("invertPhase")->load(); // Non-normalized parameters
    

    return settings;
}
