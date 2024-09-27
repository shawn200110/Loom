#include "MorphProcessor.h"


void MorphProcessor::process(const juce::dsp::ProcessContextReplacing<float>& cntxt,
                             const juce::dsp::ProcessContextReplacing<float>& cntxtA)
{
    
    auto& block = cntxt.getOutputBlock();
    auto& blockA = cntxtA.getOutputBlock();


    // Process the left channel with FFT
    applyMorph(block, blockA);
}

void MorphProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    
}

void MorphProcessor::applyMorph(juce::dsp::AudioBlock<float>& sound,
    juce::dsp::AudioBlock<float>& soundA)
{
    

    const float* frequencyData = sound.getChannelPointer(0);
    const float* frequencyDataA = soundA.getChannelPointer(0);

    int numChannels = sound.getNumChannels();
    int numSamples = sound.getNumSamples();

    morphedFFT.setSize(1, numSamples);

    float* morphedData = new float[numSamples];

    // Copy input samples into FFT buffer (zero-pad if necessary)
    for (int i = 0; i < numSamples; ++i)
    {
        morphedData[i] = (frequencyData[i] * frequencyDataA[i]) / 50;
    };

    morphedFFT.copyFrom(0, 0, morphedData, numSamples);
}
