#include "FFTProcessor.h"

FFTProcessor::FFTProcessor() :
    fft(fftOrder),
    window(fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false)
{
    // Note that the window is of length `fftSize + 1` because JUCE's windows
    // are symmetrical, which is wrong for overlap-add processing. To make the
    // window periodic, set size to 1025 but only use the first 1024 samples.
}

void FFTProcessor::reset()
{
    count = 0;
    pos = 0;

    // Zero out the circular buffers.
    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
    std::fill(inputFifoA.begin(), inputFifoA.end(), 0.0f);
    std::fill(outputFifo.begin(), outputFifo.end(), 0.0f);
}

void FFTProcessor::processBlock(float* data, float* dataA, int numSamples, bool bypassed)
{
    for (int i = 0; i < numSamples; ++i) {
        data[i] = processSample(data[i], dataA[i], bypassed);
    }
}

float FFTProcessor::processSample(float sample, float sampleA, bool bypassed)
{
    // Push the new sample value into the input FIFO.
    inputFifo[pos] = sample;
    inputFifoA[pos] = sampleA;

    // Read the output value from the output FIFO. Since it takes fftSize
    // timesteps before actual samples are read from this FIFO instead of
    // the initial zeros, the sound output is delayed by fftSize samples,
    // which we will report as our latency.
    float outputSample = outputFifo[pos];

    // Once we've read the sample, set this position in the FIFO back to
    // zero so we can add the IFFT results to it later.
    outputFifo[pos] = 0.0f;

    // Advance the FIFO index and wrap around if necessary.
    pos += 1;
    if (pos == fftSize) {
        pos = 0;
    }

    // Process the FFT frame once we've collected hopSize samples.
    count += 1;
    if (count == hopSize) {
        count = 0;
        processFrame(bypassed);
    }

    return outputSample;
}

void FFTProcessor::processFrame(bool bypassed)
{
    const float* inputPtr = inputFifo.data();
    const float* inputPtrA = inputFifoA.data();
    float* fftPtr = fftData.data();
    float* fftPtrA = fftDataA.data();

    // Copy the input FIFO into the FFT working space in two parts.
    std::memcpy(fftPtr, inputPtr + pos, (fftSize - pos) * sizeof(float));
    std::memcpy(fftPtrA, inputPtrA + pos, (fftSize - pos) * sizeof(float));
    if (pos > 0) {
        std::memcpy(fftPtr + fftSize - pos, inputPtr, pos * sizeof(float));
        std::memcpy(fftPtrA + fftSize - pos, inputPtrA, pos * sizeof(float));
    }

    // Apply the window to avoid spectral leakage.
    window.multiplyWithWindowingTable(fftPtr, fftSize);
    window.multiplyWithWindowingTable(fftPtrA, fftSize);

    if (!bypassed) {
        // Perform the forward FFT.
        fft.performRealOnlyForwardTransform(fftPtr, true);
        fft.performRealOnlyForwardTransform(fftPtrA, true);

        // Do stuff with the FFT data.
        processSpectrum(fftPtr, fftPtrA, numBins);

        // Perform the inverse FFT.
        fft.performRealOnlyInverseTransform(fftPtr);
    }

    // Apply the window again for resynthesis.
    window.multiplyWithWindowingTable(fftPtr, fftSize);

    // Scale down the output samples because of the overlapping windows.
    for (int i = 0; i < fftSize; ++i) {
        fftPtr[i] *= windowCorrection;
    }

    // Add the IFFT results to the output FIFO.
    for (int i = 0; i < pos; ++i) {
        outputFifo[i] += fftData[i + fftSize - pos];
    }
    for (int i = 0; i < fftSize - pos; ++i) {
        outputFifo[i + pos] += fftData[i];
    }
}

void FFTProcessor::processSpectrum(float* data, float* dataA, int numBins)
{
    // The spectrum data is floats organized as [re, im, re, im, ...]
    // but it's easier to deal with this as std::complex values.
    auto* cdata = reinterpret_cast<std::complex<float>*>(data);
    auto* cdataA = reinterpret_cast<std::complex<float>*>(dataA);

    for (int i = 0; i < numBins; ++i) {
        // Usually we want to work with the magnitude and phase rather
        // than the real and imaginary parts directly.
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float magnitudeA = std::abs(cdataA[i]);
        float phaseA = std::arg(cdataA[i]);

        float newMag = (magnitude + magnitudeA) / 2;
        float newPhase = (phase + phaseA) / 2;



        // This is where you'd do your spectral processing...

        // Silly example where we change the phase of each frequency bin
        // somewhat randomly. Uncomment the following line to enable.
        //phase *= float(i);

        // Convert magnitude and phase back into a complex number.
        cdata[i] = std::polar(newMag, newPhase);
    }
}
