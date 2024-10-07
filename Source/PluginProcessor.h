/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <stdio.h>

#include "DSP/FFTProcessor.h"
#include "DSP/MorphProcessor.h"
#include "DSP/FormantShiftProcessor.h"

//==============================================================================
/**
*/

template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                numSamples,
                false,   //clear everything?
                true,    //including the extra space?
                true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

enum Channel
{
    Left,    //0
    Right,   //1
    LeftAux, //2
    RightAux //3
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1,             //channel
            bufferSize,    //num samples
            false,         //keepExistingContent
            true,          //clear extra space
            true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

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

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo<BlockType> rightChannelFifo{ Channel::Right };

    SingleChannelSampleFifo<BlockType> leftAuxChannelFifo{ Channel::LeftAux };
    SingleChannelSampleFifo<BlockType> rightAuxChannelFifo{ Channel::RightAux };

    void outputToCSV(float* data, int numSamples, const std::string& fileName);
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoomAudioProcessor)


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

    //using FirstStage = juce::dsp::ProcessorChain<STFTProcessor>;

    //using FirstStage = juce::dsp::ProcessorChain<FFTProcessor>;
    
    

    //using FirstStage = juce::dsp::ProcessorChain<FFT>;

    using SecondStage = juce::dsp::ProcessorChain<MorphProcessor,        // Step 2: Morph hR and vR in the frequency domain
        FormantShiftProcessor>;    // Step 3: Formant shift on the morphed signal


    //FirstStage leftChainV, rightChainV, leftChainH, rightChainH;

    SecondStage leftChain, rightChain, leftAuxChain, rightAuxChain;
    FFTProcessor fft[2];
    MorphProcessor morphProcessor;
    FormantShiftProcessor formantProcessor;

    int fftSize = 1024;  // FFT block size

    juce::AudioBuffer<float> overlapBuffer;

    void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude, double sampleRate);
    


        


};
