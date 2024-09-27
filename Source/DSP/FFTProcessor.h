#include <JuceHeader.h>

class FFTProcessor
{
public:
    // Default constructor
    FFTProcessor() : fftSize(1024), fft(10)  // Default to 1024-point FFT (fftOrder = 10)
    {
        fftData.setSize(1, fftSize * 2);  // Allocate buffer for FFT data
    }

    FFTProcessor(int fftOrder);

    

        // Prepare function (optional)
    void prepare(double sampleRate, int samplesPerBlock);

    // Process the left and right channels
    void process(const juce::dsp::ProcessContextReplacing<float>& cntxt);

    juce::AudioBuffer<float>& getFrequencyData()  { return frequencyData; }

private:
    int fftSize;
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    juce::dsp::FFT fft;  // FFT processor
    juce::AudioBuffer<float> fftData;  // Buffer for the left channel FFT data
    juce::AudioBuffer<float> frequencyData;

    // Function to apply FFT to a single channel (left or right)
    void applyFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData);

    // Function to apply inverse FFT and copy the result back to the audio buffer
    void applyInverseFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData);

    void storeFrequencyData(const juce::AudioBuffer<float>& fftData);
};