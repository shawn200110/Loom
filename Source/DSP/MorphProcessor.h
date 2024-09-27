#include <JuceHeader.h>

class MorphProcessor
{
public:
    MorphProcessor();
    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(const juce::dsp::ProcessContextReplacing<float>& cntxt1,
                 const juce::dsp::ProcessContextReplacing<float>& cntxt2);

    juce::dsp::AudioBlock<float> getMorphedFFT() { return morphedFFT; }

    void applyMorph(juce::dsp::AudioBlock<float>& sound,
        juce::dsp::AudioBlock<float>& soundA);
    

private:
    juce::dsp::AudioBlock<float> morphedFFT;

};

