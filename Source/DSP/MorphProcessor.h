#include <JuceHeader.h>

class MorphProcessor
{
public:
    MorphProcessor();
    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(const juce::dsp::ProcessContextReplacing<float>& cntxt);

    juce::AudioBuffer<float> morph(const juce::AudioBuffer<float>& vocalFreqs,
        const juce::AudioBuffer<float>& harmonicaFreqs);
    

private:

};

