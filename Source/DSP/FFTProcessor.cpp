#include "FFTProcessor.h"
#include <math.h>
#include <algorithm>
#include <iterator>

//FFTProcessor::FFTProcessor() : fft(8), fourierf(8), fourieri(8), window(256, juce::dsp::WindowingFunction<float>::blackmanHarris) // fftSize = 2^fftOrder



// Prepare function (optional)
void FFTProcessor::prepare(double sampleRate, int samplesPerBlock)
{
    //// This can be used to allocate additional resources or set up specific parameters
    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;
}

// Process the left and right channels
void FFTProcessor::processFFT(float* bufferinput)
{
    // Get the audio blocks for left and right channels
    //auto& block = cntxt.getOutputBlock();

    //juce::ScopedNoDenormals noDenormals;

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
   
        

    // Process the left channel with FFT
    /*applyFFT(block, fftData);*/
    applyFFT(bufferinput, playhead, writehead);

    // Perform any frequency domain processing here (both fftDataLeft and fftDataRight)

    // Inverse FFT to convert back to time domain for left and right channels
    //applyInverseFFT(block, fftData);
}

void FFTProcessor::processIFFT(const juce::dsp::ProcessContextReplacing<float>& cntxt)
{
    // Get the audio blocks for left and right channels
    auto& block = cntxt.getOutputBlock();


    // Process the left channel with FFT
    /*applyInverseFFT(block, fftData);*/

    // Perform any frequency domain processing here (both fftDataLeft and fftDataRight)

    // Inverse FFT to convert back to time domain for left and right channels
    //applyInverseFFT(block, fftData);
}

void FFTProcessor::storeFrequencyData(const juce::AudioBuffer<float>& fftData)
{
    frequencyData.setSize(1, fftsize);
    // Store magnitude of FFT data into frequencyData buffer (can be modified to store real/imaginary as needed)
    for (int i = 0; i < fftsize / 2; ++i)
    {
        // For real FFT, even indices are real, odd indices are imaginary
        float realPart = fftData.getSample(0, 2 * i);    // Real part
        float imagPart = fftData.getSample(0, 2 * i + 1); // Imaginary part

        // Store magnitude (or any other transformation you need)
        float magnitude = std::sqrt(realPart * realPart + imagPart * imagPart) / fftsize;
        frequencyData.setSample(0, i, magnitude);

        DBG("freqData " << i << frequencyData.getSample(0, i));
    }
}


    // Function to apply FFT to a single channel (left or right)
//void FFTProcessor::applyFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData)
//{
//    const float* inputSamples = audioBlock.getChannelPointer(0);
//
//    // Copy input samples into FFT buffer (zero-pad if necessary)
//    for (int i = 0; i < fftSize; ++i)
//    {
//        fftData.setSample(0, i, i < audioBlock.getNumSamples() ? inputSamples[i] : 0.0f);
//    }
//
//    // Perform forward FFT
//    fft.performRealOnlyForwardTransform(fftData.getWritePointer(0));
//
//    for (int i = 0; i < fftSize; i++) {
//        std::complex<float> temp;
//        temp.real(fftData.getWritePointer(0));
//        temp.imag(fftData.getWritePointer(0))
//    }
//
//    DBG("FFT Output Data:");
//    for (int i = 0; i < fftSize; ++i)
//    {
//        if (i < 10)  // Again, limit to the first 10 values for easier viewing
//            DBG("FFT Data " << i << ": " << fftData.getSample(0, i));
//    }
//
//    storeFrequencyData(fftData);
//}

void FFTProcessor::applyFFT(float* bufferinput, int playhead, int writehead) {
    float segmented[(fftsize * 2)];
    std::fill(segmented, segmented + (fftsize * 2), 0.0f);

    int counter = 0;
    for (int i = playhead; i < fftsize; i++) {
        segmented[counter] = bufferinput[i];
        counter++;
    }

    for (int i = 0; i < playhead; i++) {
        segmented[counter] = bufferinput[i];
        counter++;
    }

    window.multiplyWithWindowingTable(segmented, fftsize);
    fourierf.performRealOnlyForwardTransform(segmented);
    for (int i = 0; i < fftsize; i++) {
        std::complex<float> temp;
        temp.real(segmented[i * 2]);
        temp.imag(segmented[i * 2 + 1]);

        mag[i] = std::abs(temp);
        phase[i] = std::arg(temp);
    }
}

// Function to apply inverse FFT and copy the result back to the audio buffer
void FFTProcessor::applyInverseFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData)
{
    //timeData.setSize(1, fftsize);
    //// Perform inverse FFT
    //fft.performRealOnlyInverseTransform(fftData.getWritePointer(0));

    //// Copy the processed FFT data back into the audio block
    //float* outputSamples = audioBlock.getChannelPointer(0);
    //for (int i = 0; i < audioBlock.getNumSamples(); ++i)
    //{
    //    outputSamples[i] = fftData.getSample(0, i);
    //}

    //timeData.copyFrom(0, 0, outputSamples, audioBlock.getNumSamples());
}