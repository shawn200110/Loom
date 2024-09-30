#include <JuceHeader.h>

class FFTProcessor
{
public:
    // Default constructor
    FFTProcessor() : fourierf(log2(fftsize)), fourieri(log2(fftsize)), window(fftsize, juce::dsp::WindowingFunction<float>::hann, false, 0.0f)
    {

        std::fill(segmented, segmented + (fftsize * 2), 0.0f);
        std::fill(leftbuf, leftbuf + (fftsize), 0.0f);
        std::fill(rightbuf, rightbuf + (fftsize), 0.0f);
        std::fill(leftauxbuf, leftauxbuf + (fftsize), 0.0f);
        std::fill(rightauxbuf, rightauxbuf + (fftsize), 0.0f);
        std::fill(leftbufout, leftbufout + (fftsize), 0.0f);
        std::fill(rightbufout, rightbufout + (fftsize), 0.0f);
    };


        // Prepare function (optional)
    void prepare(double sampleRate, int samplesPerBlock);

    // Process the left and right channels
    void processFFT(juce::AudioBuffer<float> bufferinput, int channel);
    void processIFFT(float* m, float* p, int channel);

    // Function to apply inverse FFT and copy the result back to the audio buffer
    

    juce::AudioBuffer<float>& getFrequencyData()  { return frequencyData; }
    juce::AudioBuffer<float>& getTimeData() { return timeData; }

    float* getOutputData(int channel);

    float* getMagnitude() { return mag; }
    float* getPhase() { return phase; }

    
    

private:
    double sampleRate = 44100.0;
    int samplesPerBlock = 4096;

    static constexpr int fftsize = 512;
    float leftbuf[fftsize], rightbuf[fftsize], leftauxbuf[fftsize], rightauxbuf[fftsize],leftbufout[fftsize], rightbufout[fftsize], leftauxbufout[fftsize], rightauxbufout[fftsize]; // circular buffers
    int playheadL=0, writeheadL=0, playheadR=0, writeheadR=0,
        playheadLA=0, writeheadLA=0, playheadRA=0, writeheadRA = 0;
    juce::dsp::FFT fourierf, fourieri;
    juce::dsp::WindowingFunction<float> window;
    int windowL = 0, windowR = 0, windowLA = 0, windowRA = 0;

    float mag[fftsize];
    float phase[fftsize];
    float bufferoutput[fftsize];

    float segmented[(fftsize * 2)];

    juce::AudioBuffer<float> fftData;  // Buffer for the left channel FFT data
    juce::AudioBuffer<float> frequencyData;
    juce::AudioBuffer<float> timeData;

    // Function to apply FFT to a single channel (left or right)
    //void applyFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData);
    void applyFFT(juce::AudioBuffer<float>  bufferinput, int channel);
    void applyInverseFFT(float* m, float* p, int channel);

    void updatePlayhead(int channel, int after);
    void updateWritehead(int channel, int after);
    void updateWindowLoc(int channel);
    int selectPlayhead(int channel);
    int selectWritehead(int channel);
    int selectWindowLoc(int channel);
    float* selectBTP(int channel);
    float* selectBufOut(int channel);

    
    

    void storeFrequencyData(const juce::AudioBuffer<float>& fftData);
};