#include <JuceHeader.h>

class FFTProcessor
{
public:
    // Default constructor
    FFTProcessor() : fourierf(log2(fftsize)), fourieri(log2(fftsize)), window(fftsize, juce::dsp::WindowingFunction<float>::hann, false, 0.0f)
    {
        playhead = 0;
        std::fill(leftbuf, leftbuf + (fftsize), 0.0f);
        std::fill(rightbuf, rightbuf + (fftsize), 0.0f);
        std::fill(leftbufout, leftbufout + (fftsize), 0.0f);
        std::fill(rightbufout, rightbufout + (fftsize), 0.0f);
    };


        // Prepare function (optional)
    void prepare(double sampleRate, int samplesPerBlock);

    // Process the left and right channels
    void processFFT(float* bufferinput);
    void processIFFT(const juce::dsp::ProcessContextReplacing<float>& cntxt);

    // Function to apply inverse FFT and copy the result back to the audio buffer
    void applyInverseFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData);

    juce::AudioBuffer<float>& getFrequencyData()  { return frequencyData; }
    juce::AudioBuffer<float>& getTimeData() { return timeData; }

    float* getMagnitude() { return mag; }
    float* getPhase() { return phase; }

private:
    double sampleRate = 44100.0;
    int samplesPerBlock = 4096;

    static constexpr int fftsize = 256;
    float leftbuf[fftsize], rightbuf[fftsize], leftbufout[fftsize], rightbufout[fftsize]; // circular buffers
    int playhead, writehead = 0;
    juce::dsp::FFT fourierf, fourieri;
    juce::dsp::WindowingFunction<float> window;
    int windowL = 0, windowR = 0;

    float mag[fftsize];
    float phase[fftsize];

    juce::AudioBuffer<float> fftData;  // Buffer for the left channel FFT data
    juce::AudioBuffer<float> frequencyData;
    juce::AudioBuffer<float> timeData;

    // Function to apply FFT to a single channel (left or right)
    //void applyFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData);
    void applyFFT(float* bufferinput, int playhead, int writehead);

    
    

    void storeFrequencyData(const juce::AudioBuffer<float>& fftData);
};