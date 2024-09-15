/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <stdio.h>

#include "DSP/ProcessorClasses.h"

//==============================================================================
/**
*/

class LoomAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    LoomAudioProcessor();
    ~LoomAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    juce::AudioProcessorValueTreeState apvts{ *this,nullptr,
        "Parameters",createParameterLayout() };



private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoomAudioProcessor)


    STFTProcessor vocalSTFT, harmonicaSTFT;  // STFT objects for vocal and harmonica signals

    // Morphing function: define how the signals should be morphed in the frequency domain
    juce::AudioBuffer<float> morph(const juce::AudioBuffer<float>& vocalFreqs,
        const juce::AudioBuffer<float>& harmonicaFreqs)
    {
        // Example: multiply magnitudes or blend them
        juce::AudioBuffer<float> result;
        for (int i = 0; i < vocalFreqs.getNumSamples(); ++i)
        {
            result.setSample(0, i, vocalFreqs.getSample(0, i) * harmonicaFreqs.getSample(0, i));
        }
        return result;
    }

    // Formant shifting function: modify the frequency bins to shift formants
    juce::AudioBuffer<float> formantShift(const juce::AudioBuffer<float>& freqDomainSignal)
    {
        // Example: shift the formant structure (e.g., using spectral envelope modification)
        return freqDomainSignal;  // Implement formant shifting here
    }

    using FirstStage = juce::dsp::ProcessorChain<STFTProcessor>;

    using SecondStage = juce::dsp::ProcessorChain<MorphProcessor,        // Step 2: Morph hR and vR in the frequency domain
        FormantShiftProcessor,    // Step 3: Formant shift on the morphed signal
        InverseSTFTProcessor>;

    FirstStage leftChainV, rightChainV, leftChainH, rightChainH;

    SecondStage leftChain, rightChain;

        


};
