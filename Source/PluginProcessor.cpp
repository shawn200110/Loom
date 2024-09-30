/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LoomAudioProcessor::LoomAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withInput ("Aux Input", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

LoomAudioProcessor::~LoomAudioProcessor()
{
}

//==============================================================================
const juce::String LoomAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LoomAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LoomAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LoomAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LoomAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LoomAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LoomAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LoomAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LoomAudioProcessor::getProgramName (int index)
{
    return {};
}

void LoomAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void LoomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec{};
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    fftProcessor.prepare(sampleRate, samplesPerBlock);

    int fftBlockSize = 512;
    leftChannelFifo.prepare(fftBlockSize);
    rightChannelFifo.prepare(fftBlockSize);
    /*leftAuxChannelFifo.prepare(fftBlockSize);
    rightAuxChannelFifo.prepare(fftBlockSize);*/

    overlapBuffer.setSize(1, overlapSize);  // 1 channel, overlapSize samples
    overlapBuffer.clear();
}

void LoomAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LoomAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void LoomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    //DBG("buffersize: " << buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);

    generateSineWave(buffer, 440.0f, 1.0f, getSampleRate());

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
    //leftAuxChannelFifo.update(buffer);
    //rightAuxChannelFifo.update(buffer);
    

    //if (overlapBuffer.getNumSamples() != overlapSize)
    //{
    //    overlapBuffer.setSize(1, overlapSize);  // Initialize with overlap size
    //    overlapBuffer.clear();
    //}


    // Check if all FIFOs have a complete block ready for FFT processing
    //if (leftChannelFifo.getNumCompleteBuffersAvailable() > 0 &&
    //    rightChannelFifo.getNumCompleteBuffersAvailable() > 0 &&
    //    leftAuxChannelFifo.getNumCompleteBuffersAvailable() > 0 &&
    //    rightAuxChannelFifo.getNumCompleteBuffersAvailable() > 0)
    if (leftChannelFifo.getNumCompleteBuffersAvailable() > 0 &&
        rightChannelFifo.getNumCompleteBuffersAvailable() > 0)
    {
        // Create buffers to hold the FFT input
        juce::AudioBuffer<float> leftFftInput, rightFftInput, leftAuxFftInput, rightAuxFftInput;

        // Pull the complete buffer from each FIFO
        leftChannelFifo.getAudioBuffer(leftFftInput);
        rightChannelFifo.getAudioBuffer(rightFftInput);
        /*leftAuxChannelFifo.getAudioBuffer(leftAuxFftInput);
        rightAuxChannelFifo.getAudioBuffer(rightAuxFftInput);*/

        //auto* LPtr = leftFftInput.getReadPointer(0);
        //auto* LAPtr = leftAuxFftInput.getReadPointer(0);

        //for (int i = 0; i < 10; ++i)
        //{
        //   DBG("LPtr " << i << ": " << LPtr[i]);
        //   //DBG("LAPtr " << i << ": " << LAPtr[i]);
        // };

        //fftProcessor.processFFT(leftFftInput,0);
        fftProcessor.processFFT(leftFftInput, 0);
        float* magL = fftProcessor.getMagnitude();
        float* phaseL = fftProcessor.getPhase();

        for (int i = 0; i < 100; ++i)
        {
           DBG("magL " << i << ": " << magL[i]);
        }; 

        //fftProcessor.processFFT(rightFftInput,1);
        fftProcessor.processFFT(rightFftInput, 1);
        float* magR = fftProcessor.getMagnitude();
        float* phaseR = fftProcessor.getPhase();

        /*for (int i = 0; i < 10; ++i)
        {
            DBG("magR " << i << ": " << magR[i]);
        };*/


        //fftProcessor.processFFT(leftAuxFftInput,2);
        //fftProcessor.processFFT(buffer, 2);
        //float* magLA = fftProcessor.getMagnitude();
        //float* phaseLA = fftProcessor.getPhase();

        //for (int i = 0; i < 10; ++i)
        //{
        //    DBG("magLA " << i << ": " << magLA[i]);
        //};

        //fftProcessor.processFFT(rightAuxFftInput,3);
        //fftProcessor.processFFT(buffer, 3);
        //float* magRA = fftProcessor.getMagnitude();
        //float* phaseRA = fftProcessor.getPhase();







        // Morph

        // Formant Shift


        // Inverse FFT
        fftProcessor.processIFFT(magL, phaseL, 0);
        float* outL = fftProcessor.getOutputData(0);

        /*for (int i = 0; i < 100; ++i)
        {
            DBG("outL " << i << ": " << outL[i]);
        };*/

        fftProcessor.processIFFT(magR, phaseR, 1);
        float* outR = fftProcessor.getOutputData(1);
        //fftProcessor.processIFFT(magLA, phaseLA, 2);
        //float* outLA = fftProcessor.getOutputData(2);
        //fftProcessor.processIFFT(magRA, phaseRA,3);
        //float* outRA = fftProcessor.getOutputData(3);

        
        //for (int i = 0; i < 10; ++i)
        //{
        //    DBG("outRA " << i << ": " << outRA[i]);
        //};

        // Get write pointer to the right channel (channel 1)
        float* leftChannelData = buffer.getWritePointer(0);
        float* rightChannelData = buffer.getWritePointer(1);

        //// Copy the float array to the right channel (could use different data if needed)
        const float* channelData = buffer.getWritePointer(0);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            leftChannelData[sample] = outL[sample];
            rightChannelData[sample] = outR[sample];
        }

        for (int sample = 0; sample < 100; ++sample)
        {
            //leftChannelData[sample] = outL[sample];
            DBG("ChannelData " << sample << ": " << channelData[sample]);
            DBG("NumSamplesInBuffer" << buffer.getNumSamples());
            DBG("outR: " << outR[sample]);
            DBG("outL: " << outR[sample]);
            //rightChannelData[sample] = outR[sample];  // Output myFloatArray to the right channel

        }
          


    }
}

//==============================================================================
bool LoomAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LoomAudioProcessor::createEditor()
{
    return new LoomAudioProcessorEditor (*this);
}

//==============================================================================
void LoomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LoomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout
LoomAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("numFBs", "# Freq Bands", juce::NormalisableRange <float>(8.f,64.f,4.f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("fmt", "Formant Shift", juce::NormalisableRange <float>(-12.f, 12.f, 0.1f, 1.f),1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("atk", "Attack", juce::NormalisableRange <float>(0.f, 0.1f, 0.01f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("rls", "Release", juce::NormalisableRange <float>(0.01f, 0.5f, 0.01f, 1.f), 1.f));


    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoomAudioProcessor();
}

void LoomAudioProcessor::generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude, double sampleRate)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Fill the buffer with a sine wave on all channels
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Calculate the time `t` in seconds for the current sample
            float t = static_cast<float>(sample) / sampleRate;

            // Generate the sine wave for this sample
            channelData[sample] = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * t);
        }
    }
}
