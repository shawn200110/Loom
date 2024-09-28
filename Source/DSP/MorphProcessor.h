#include <JuceHeader.h>

class MorphProcessor
{
public:
    MorphProcessor() : fftSize(4096)
    {
        morphedFFT.setSize(1, fftSize);
    };
    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(const juce::dsp::ProcessContextReplacing<float>& cntxt1,
                 const juce::dsp::ProcessContextReplacing<float>& cntxt2);

    juce::AudioBuffer<float>& getMorphedFFT() { return morphedFFT; }

    void applyMorph(juce::dsp::AudioBlock<float>& sound,
        juce::dsp::AudioBlock<float>& soundA);
    

private:
    int fftSize;
    juce::AudioBuffer<float> morphedFFT;

};

