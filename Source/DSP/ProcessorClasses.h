class STFTProcessor
{
public:
    STFTProcessor();
    void prepare(juce::dsp::ProcessSpec spec);
    void process(juce::dsp::ProcessContextReplacing<float> cntxt);
    /*void initialize(double sampleRate, int samplesPerBlock);
    void performForwardTransform(const float* inputBuffer);
    void performInverseTransform(juce::AudioBuffer<float>& outputBuffer);
    const juce::AudioBuffer<float>& getFrequencyBins() const;*/

private:
    /*int fftSize, hopSize;
    juce::dsp::FFT fft;
    juce::AudioBuffer<float> window;
    juce::AudioBuffer<float> timeDomainBuffer;
    juce::AudioBuffer<std::complex<float>> freqDomainBuffer;
    juce::AudioBuffer<float> outputBuffer;*/
};

class MorphProcessor
{
public:
    MorphProcessor();
    void prepare(juce::dsp::ProcessSpec spec);
    void process(juce::dsp::ProcessContextReplacing<float> cntxt);

    juce::AudioBuffer<float> morph(const juce::AudioBuffer<float>& vocalFreqs,
        const juce::AudioBuffer<float>& harmonicaFreqs);
    

private:

};


class FormantShiftProcessor
{
public:
    FormantShiftProcessor();
    void prepare(juce::dsp::ProcessSpec spec);
    void process(juce::dsp::ProcessContextReplacing<float> cntxt);


private:

};


class InverseSTFTProcessor
{
public:
      InverseSTFTProcessor();
      void prepare(juce::dsp::ProcessSpec spec);
      void process(juce::dsp::ProcessContextReplacing<float> cntxt);


private:

}; 