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

void FFTProcessor::processBlock(float* data, float* dataA, int numSamples, ChainSettings settings)
{
    for (int i = 0; i < numSamples; ++i) {
        data[i] = processSample(data[i], dataA[i], settings);
    }
}

// Function that counts samples, and then calls processFrame once there's enough samples gathered to perform the FFT
float FFTProcessor::processSample(float sample, float sampleA, ChainSettings settings)
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
        processFrame(settings);
    }

    return outputSample;
}

// Function that performs the FFT and calls processSpectrum
void FFTProcessor::processFrame(ChainSettings settings)
{
    const float* inputPtr = inputFifo.data();
    const float* inputPtrA = inputFifoA.data();
    float* fftPtr = fftData.data();
    float* fftPtrA = fftDataA.data();

    bool bypassed = settings.bypassed;

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
        processSpectrum(fftPtr, fftPtrA, numBins, settings);

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

// Function that calls the phase/magnitude processors
void FFTProcessor::processSpectrum(float* data, float* dataA, int numBins, ChainSettings settings)
{
    // The spectrum data is floats organized as [re, im, re, im, ...]
    // but it's easier to deal with this as std::complex values.
    auto* cdata = reinterpret_cast<std::complex<float>*>(data);
    auto* cdataA = reinterpret_cast<std::complex<float>*>(dataA);

    int magMethod = settings.magProcessing;
    int phaseMethod = settings.phaseProcessing;


    // Choose method of magnitude processing
    switch (magMethod)
    {
    case magProcessing::addM: addAverageMagnitude(cdata, cdataA, numBins, settings); break;
    case magProcessing::subtract: subtractAverageMagnitude(cdata, cdataA, numBins, settings); break;
    case magProcessing::multiply: multiplyAverageMagnitude(cdata, cdataA, numBins, settings); break;
    case magProcessing::divide: divideAverageMagnitude(cdata, cdataA, numBins, settings); break;
    case magProcessing::linearBlend: linearBlendMagnitude(cdata, cdataA, numBins, settings); break;
    case magProcessing::allPass: break;
    }

    // Choose method of phase processing
    switch (phaseMethod)
    {
    case phaseProcessing::addP: averagePhase(cdata, cdataA, numBins, settings); break;
    case phaseProcessing::linear: linearPhase(cdata, numBins, settings); break;
    case phaseProcessing::linearNatural: linearNaturalPhase(cdata, cdataA, numBins, settings); break;
    case phaseProcessing::smoothStep: smoothStepPhase(cdata, cdataA, numBins, settings); break;
    case phaseProcessing::preserveMainIn: break;
    case phaseProcessing::preserveAuxIn: preserveAuxInPhase(cdata, cdataA, numBins, settings); break;
    }

    if (settings.invertPhase) invertPhase(cdata, numBins, settings);
}


// Phase Processing Functions
void FFTProcessor::averagePhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings)
{
    float morphFactor = settings.morphFactor;
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float magnitudeA = std::abs(cdataA[i]);
        float phaseA = std::arg(cdataA[i]);

        float morphedPhase = (phase * morphFactor) + (phaseA * (1.0f - morphFactor));

        cdata[i] = std::polar(magnitude, morphedPhase);
    }
}
void FFTProcessor::linearPhase(std::complex<float>* cdata, int numBins, ChainSettings settings)
{
    std::vector<float> linearPhase = linspace(-3.14f, 3.14f, numBins);

    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = linearPhase[i];

        // Wrap phase to the range [-pi, pi] to avoid discontinuities
        if (phase > 3.14f) phase -= 2.0f * 3.14f;
        if (phase < -3.14f) phase += 2.0f * 3.14f;

        cdata[i] = std::polar(magnitude, phase);
    }
}
void FFTProcessor::linearNaturalPhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings) 
{
    // Generate a linear phase ramp from -pi to pi
    std::vector<float> linearPhase = linspace(-3.14f, 3.14f, numBins);

    float morphFactor = settings.morphFactor;  // Value between 0 and 1

    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float phaseA = std::arg(cdataA[i]);

        // Compute the average of the two input phases
        float averageMorphedPhase = (phase * morphFactor) + (phaseA * (1.0f - morphFactor));
        

        // Blend between the forced linear phase and the average phase
        float blendedPhase = (1.0f - morphFactor) * averageMorphedPhase + morphFactor * linearPhase[i];

        // Wrap phase to the range [-pi, pi] to avoid discontinuities
        if (blendedPhase > 3.14f) blendedPhase -= 2.0f * 3.14f;
        if (blendedPhase < -3.14f) blendedPhase += 2.0f * 3.14f;

        // Reconstruct the complex number with the morphed magnitude and blended phase
        cdata[i] = std::polar(magnitude, blendedPhase);
    }
}
void FFTProcessor::smoothStepPhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings) 
{
    float morphFactor = settings.morphFactor;
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float magnitudeA = std::abs(cdataA[i]);
        float phaseA = std::arg(cdataA[i]);
        float blendCurve = 3 * morphFactor * morphFactor - 2 * morphFactor * morphFactor * morphFactor;  // Smoothstep function
        float morphedPhase = blendCurve * phase + (1 - blendCurve) * phaseA;

        cdata[i] = std::polar(magnitude, morphedPhase);
    }
}
void FFTProcessor::preserveAuxInPhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings) 
{
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phaseA = std::arg(cdataA[i]);
        cdata[i] = std::polar(magnitude, phaseA);
    }
}
void FFTProcessor::invertPhase(std::complex<float>* cdata, int numBins, ChainSettings settings)
{
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        cdata[i] = std::polar(magnitude, -phase);
    }
}

// Magnitude Processing Functions
void FFTProcessor::addAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings)
{
    float morphFactor = settings.morphFactor;
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float magnitudeA = std::abs(cdataA[i]);
        float phaseA = std::arg(cdataA[i]);

        float morphedMagnitude = (magnitude * morphFactor) + (magnitudeA * (1.0f - morphFactor));

        cdata[i] = std::polar(morphedMagnitude, phase);
    }
}
void FFTProcessor::subtractAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings)
{
    float morphFactor = settings.morphFactor;
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float magnitudeA = std::abs(cdataA[i]);
        float phaseA = std::arg(cdataA[i]);

        float morphedMagnitude = std::abs((magnitude * morphFactor) - (magnitudeA * (1.0f - morphFactor)));

        cdata[i] = std::polar(morphedMagnitude, phase);
    }
}
void FFTProcessor::multiplyAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings)
{
    float morphFactor = settings.morphFactor;
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float magnitudeA = std::abs(cdataA[i]);
        float phaseA = std::arg(cdataA[i]);

        float morphedMagnitude = std::abs((magnitude * morphFactor) * (magnitudeA * (1.0f - morphFactor)));

        float normalizationFactor = std::max(magnitude * magnitudeA, 1.0f);
        morphedMagnitude /= normalizationFactor;

        cdata[i] = std::polar(morphedMagnitude, phase);
    }
}
void FFTProcessor::divideAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings)
{
    float morphFactor = settings.morphFactor;
    for (int i = 0; i < numBins; ++i) {
        float magnitude = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        float magnitudeA = std::abs(cdataA[i]);
        float phaseA = std::arg(cdataA[i]);

        // Avoid division by zero by clamping magnitudeA to a small positive value
        float safeMagnitudeA = std::max(magnitudeA * (1.0f - morphFactor), 1e-6f);

        // Perform division with morphing factors
        float morphedMagnitude = (magnitude * morphFactor) / safeMagnitudeA;

        // Optional: Clamp to avoid extremely large values
        morphedMagnitude = std::min(morphedMagnitude, 1.0f);  // Assuming normalized input

        cdata[i] = std::polar(morphedMagnitude, phase);
    }
}
void FFTProcessor::linearBlendMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings)
{
    // Create a linear blending curve from 0 to 1 across the frequency bins
    std::vector<float> blendCurve = linspace(0.0f, 1.0f, numBins);

    for (int i = 0; i < numBins; ++i) {
        float magnitudeA = std::abs(cdataA[i]);
        float magnitudeB = std::abs(cdata[i]);

        // Use the blend curve to crossfade between magnitudes
        float blendedMagnitude = blendCurve[i] * magnitudeA + (1.0f - blendCurve[i]) * magnitudeB;

        // Preserve the original phase from cdataA (or you can mix the phases if desired)
        float phase = std::arg(cdata[i]);

        // Reconstruct the complex number with the blended magnitude and phase
        cdata[i] = std::polar(blendedMagnitude, phase);
    }
}

// helpers
std::vector<float> FFTProcessor::linspace(float start, float end, int numPoints) {
    std::vector<float> result(numPoints);
    if (numPoints == 1) {
        result[0] = start;
        return result;
    }

    float step = (end - start) / (numPoints - 1);

    for (int i = 0; i < numPoints; ++i) {
        result[i] = start + step * i;
    }

    return result;
}


