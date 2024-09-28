#include <JuceHeader.h>

class FFTProcessor
{
public:
    // Default constructor
    FFTProcessor() : fftSize(4096), fft(12)  // Default to 1024-point FFT (fftOrder = 10)
    {
        fftData.setSize(1, fftSize * 2);  // Allocate buffer for FFT data
    }

    FFTProcessor(int fftOrder);

    

        // Prepare function (optional)
    void prepare(double sampleRate, int samplesPerBlock);

    // Process the left and right channels
    void processFFT(const juce::dsp::ProcessContextReplacing<float>& cntxt);
    void processIFFT(const juce::dsp::ProcessContextReplacing<float>& cntxt);

    // Function to apply inverse FFT and copy the result back to the audio buffer
    void applyInverseFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData);

    juce::AudioBuffer<float>& getFrequencyData()  { 
        return frequencyData; }
    juce::AudioBuffer<float>& getTimeData() { return timeData; }

private:
    int fftSize;
    double sampleRate = 44100.0;
    int samplesPerBlock = 4096;

    juce::dsp::FFT fft;  // FFT processor
    juce::AudioBuffer<float> fftData;  // Buffer for the left channel FFT data
    juce::AudioBuffer<float> frequencyData;
    juce::AudioBuffer<float> timeData;

    // Function to apply FFT to a single channel (left or right)
    void applyFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData);

    
    

    void storeFrequencyData(const juce::AudioBuffer<float>& fftData);
};



/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
//==============================================================================
/**
*/


class Fourier4AudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    Fourier4AudioProcessor();
    ~Fourier4AudioProcessor() override;

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
    static constexpr int fftsize = 256;
    float leftbuf[fftsize], rightbuf[fftsize], leftbufout[fftsize], rightbufout[fftsize]; // circular buffers
    int playheadL, playheadR, writeheadL = 0, writeheadR = 0;
    dsp::FFT fourierf, fourieri;
    dsp::WindowingFunction<float> window;
    int windowL = 0, windowR = 0;
    void processCorrect(float* bufferinput, float* bufferoutput, int playhead, int writehead);
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fourier4AudioProcessor)
};