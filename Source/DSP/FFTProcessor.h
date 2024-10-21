#pragma once

#include <JuceHeader.h>

/**
  STFT analysis and resynthesis of audio data.

  Each channel should have its own FFTProcessor.
 */
struct ChainSettings {
    float bypassed{ 0 };
    float morphFactor{ 0.5 };
    float formantShiftFactor{ 0 };
    float magProcessing{ 0 };
    float phaseProcessing{ 0 };
    float invertPhase{ 0 };
};

enum magProcessing
{
    addM,                // 0
    subtract,           // 1
    multiply,           // 2
    divide,             // 3
    linearBlend,        // 4
    allPass             // 5
};

enum phaseProcessing
{
    addP,                // 0
    linear,              // 1
    linearNatural,       // 2
    smoothStep,          // 3
    preserveMainIn,      // 4
    preserveAuxIn,       // 5
};

class FFTProcessor
{
public:
    FFTProcessor();

    int getLatencyInSamples() const { return fftSize; }

    void reset();
    float processSample(float sample, float sampleA, ChainSettings settings);
    void processBlock(float* data, float* dataA, int numSamples, ChainSettings settings);

private:

    void processFrame(ChainSettings settings);
    void processSpectrum(float* data, float* dataA, int numBins, ChainSettings settings);

    // Phase Processing Functions
    void averagePhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void linearPhase(std::complex<float>* cdata, int numBins, ChainSettings settings);
    void linearNaturalPhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void smoothStepPhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void preserveAuxInPhase(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void invertPhase(std::complex<float>* cdata, int numBins, ChainSettings settings);

    std::vector<float> linspace(float start, float end, int numPoints); // helper

    // Magnitude Processing Functions
    void addAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void subtractAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void multiplyAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void divideAverageMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);
    void linearBlendMagnitude(std::complex<float>* cdata, std::complex<float>* cdataA, int numBins, ChainSettings settings);


    // The FFT has 2^order points and fftSize/2 + 1 bins.
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;      // 1024 samples
    static constexpr int numBins = fftSize / 2 + 1;    // 513 bins
    static constexpr int overlap = 4;                  // 75% overlap
    static constexpr int hopSize = fftSize / overlap;  // 256 samples

    // Gain correction for using Hann window with 75% overlap.
    static constexpr float windowCorrection = 2.0f / 3.0f;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    // Counts up until the next hop.
    int count = 0;

    // Write position in input FIFO and read position in output FIFO.
    int pos = 0;

    // Circular buffers for incoming and outgoing audio data.
    std::array<float, fftSize> inputFifo, inputFifoA;
    std::array<float, fftSize> outputFifo;

    // The FFT working space. Contains interleaved complex numbers.
    std::array<float, fftSize * 2> fftData, fftDataA;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTProcessor)
};
