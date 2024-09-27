#include "FFTProcessor.h"

FFTProcessor::FFTProcessor(int fftOrder)
    : fft(fftOrder), fftSize(1 << fftOrder)  // fftSize = 2^fftOrder
{
    // Allocate buffers for FFT (2x size for real and imaginary parts)
    fftData.setSize(1, fftSize * 2);
}

// Prepare function (optional)
void FFTProcessor::prepare(double sampleRate, int samplesPerBlock)
{
    //// This can be used to allocate additional resources or set up specific parameters
    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;
}

// Process the left and right channels
void FFTProcessor::process(const juce::dsp::ProcessContextReplacing<float>& cntxt)
{
    // Get the audio blocks for left and right channels
    auto& block = cntxt.getOutputBlock();
        

    // Process the left channel with FFT
    applyFFT(block, fftData);

    // Perform any frequency domain processing here (both fftDataLeft and fftDataRight)

    // Inverse FFT to convert back to time domain for left and right channels
    //applyInverseFFT(block, fftData);
}

void FFTProcessor::storeFrequencyData(const juce::AudioBuffer<float>& fftData)
{
    frequencyData.setSize(1, fftSize / 2);
    // Store magnitude of FFT data into frequencyData buffer (can be modified to store real/imaginary as needed)
    for (int i = 0; i < fftSize / 2; ++i)
    {
        // For real FFT, even indices are real, odd indices are imaginary
        float realPart = fftData.getSample(0, 2 * i);    // Real part
        float imagPart = fftData.getSample(0, 2 * i + 1); // Imaginary part

        // Store magnitude (or any other transformation you need)
        float magnitude = std::sqrt(realPart * realPart + imagPart * imagPart);
        frequencyData.setSample(0, i, magnitude);
    }
}


    // Function to apply FFT to a single channel (left or right)
void FFTProcessor::applyFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData)
{
    const float* inputSamples = audioBlock.getChannelPointer(0);

    // Copy input samples into FFT buffer (zero-pad if necessary)
    for (int i = 0; i < fftSize; ++i)
    {
        fftData.setSample(0, i, i < audioBlock.getNumSamples() ? inputSamples[i] : 0.0f);
    }

    // Perform forward FFT
    fft.performRealOnlyForwardTransform(fftData.getWritePointer(0));

    DBG("FFT Output Data:");
    for (int i = 0; i < fftSize; ++i)
    {
        if (i < 10)  // Again, limit to the first 10 values for easier viewing
            DBG("FFT Data " << i << ": " << fftData.getSample(0, i));
    }

    storeFrequencyData(fftData);
}

// Function to apply inverse FFT and copy the result back to the audio buffer
void FFTProcessor::applyInverseFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData)
{
    // Perform inverse FFT
    fft.performRealOnlyInverseTransform(fftData.getWritePointer(0));

    // Copy the processed FFT data back into the audio block
    float* outputSamples = audioBlock.getChannelPointer(0);
    for (int i = 0; i < audioBlock.getNumSamples(); ++i)
    {
        outputSamples[i] = fftData.getSample(0, i);
    }
}