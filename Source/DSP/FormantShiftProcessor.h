#include <JuceHeader.h>

class FormantShiftProcessor
{
public:
    FormantShiftProcessor();
    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(const juce::dsp::ProcessContextReplacing<float>& cntxt);


private:

};