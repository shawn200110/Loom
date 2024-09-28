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

    int fftBlockSize = 4096;
    leftChannelFifo.prepare(fftBlockSize);
    rightChannelFifo.prepare(fftBlockSize);
    leftAuxChannelFifo.prepare(fftBlockSize);
    rightAuxChannelFifo.prepare(fftBlockSize);

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

    juce::dsp::AudioBlock<float> block(buffer);

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
    leftAuxChannelFifo.update(buffer);
    rightAuxChannelFifo.update(buffer);
    

    if (overlapBuffer.getNumSamples() != overlapSize)
    {
        overlapBuffer.setSize(1, overlapSize);  // Initialize with overlap size
        overlapBuffer.clear();
    }



    // Check if all FIFOs have a complete block ready for FFT processing
    if (leftChannelFifo.getNumCompleteBuffersAvailable() > 0 &&
        rightChannelFifo.getNumCompleteBuffersAvailable() > 0 &&
        leftAuxChannelFifo.getNumCompleteBuffersAvailable() > 0 &&
        rightAuxChannelFifo.getNumCompleteBuffersAvailable() > 0)
    {
        // Create buffers to hold the FFT input
        juce::AudioBuffer<float> leftFftInput, rightFftInput, leftAuxFftInput, rightAuxFftInput;

        // Pull the complete buffer from each FIFO
        leftChannelFifo.getAudioBuffer(leftFftInput);
        rightChannelFifo.getAudioBuffer(rightFftInput);
        leftAuxChannelFifo.getAudioBuffer(leftAuxFftInput);
        rightAuxChannelFifo.getAudioBuffer(rightAuxFftInput);

        /*auto leftBlock = block.getSingleChannelBlock(0);
        auto rightBlock = block.getSingleChannelBlock(1);
        auto leftAuxBlock = block.getSingleChannelBlock(2);
        auto rightAuxBlock = block.getSingleChannelBlock(3);*/

        juce::dsp::AudioBlock<float> leftBlock(leftFftInput);
        juce::dsp::AudioBlock<float> rightBlock(rightFftInput);
        juce::dsp::AudioBlock<float> leftAuxBlock(leftAuxFftInput);
        juce::dsp::AudioBlock<float> rightAuxBlock(rightAuxFftInput);

        juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        juce::dsp::ProcessContextReplacing<float> leftAuxContext(leftAuxBlock);
        juce::dsp::ProcessContextReplacing<float> rightAuxContext(rightAuxBlock);

        // Perform FFT on each of the accumulated blocks
        // get fft of all i/p channels
        fftProcessor.processFFT(leftContext);
        juce::AudioBuffer<float>& frequencyDataL = fftProcessor.getFrequencyData();
        fftProcessor.processFFT(rightContext);
        juce::AudioBuffer<float>& frequencyDataR = fftProcessor.getFrequencyData();
        fftProcessor.processFFT(leftAuxContext);
        juce::AudioBuffer<float>& frequencyDataLA = fftProcessor.getFrequencyData();
        fftProcessor.processFFT(rightAuxContext);
        juce::AudioBuffer<float>& frequencyDataRA = fftProcessor.getFrequencyData();

        //// Assuming you have two frequency buffers from the FFT of two signals
        juce::dsp::AudioBlock<float> frequencyBlockL(frequencyDataL);
        juce::dsp::AudioBlock<float> frequencyBlockLA(frequencyDataLA);
        juce::dsp::AudioBlock<float> frequencyBlockR(frequencyDataR);
        juce::dsp::AudioBlock<float> frequencyBlockRA(frequencyDataRA);

        //// Create contexts for both
        juce::dsp::ProcessContextReplacing<float> frequencyContextL(frequencyBlockL);
        juce::dsp::ProcessContextReplacing<float> frequencyContextLA(frequencyBlockLA);
        juce::dsp::ProcessContextReplacing<float> frequencyContextR(frequencyBlockR);
        juce::dsp::ProcessContextReplacing<float> frequencyContextRA(frequencyBlockRA);

        morphProcessor.process(frequencyContextL, frequencyContextLA);
        juce::AudioBuffer<float>& morphedFFTL = morphProcessor.getMorphedFFT();
        morphProcessor.process(frequencyContextR, frequencyContextRA);
        juce::AudioBuffer<float>& morphedFFTR = morphProcessor.getMorphedFFT();

        juce::dsp::AudioBlock<float> morphedBlockL(morphedFFTL);
        juce::dsp::AudioBlock<float> morphedBlockR(morphedFFTR);

        juce::dsp::ProcessContextReplacing<float> morphedContextL(morphedBlockL);
        juce::dsp::ProcessContextReplacing<float> morphedContextR(morphedBlockR);

        // Perform IFFT to convert frequency data back to time domain for each channel
        fftProcessor.processIFFT(morphedContextL);
        juce::AudioBuffer<float>& outputDataL = fftProcessor.getTimeData();
        fftProcessor.processIFFT(morphedContextR);
        juce::AudioBuffer<float>& outputDataR = fftProcessor.getTimeData();

        // Determine the number of samples to copy based on the smaller buffer size
        int numSamplesToCopy = juce::jmin(buffer.getNumSamples(), outputDataL.getNumSamples());

        // Write the processed data back to the output buffer (only copy the number of samples that fit)
        //buffer.copyFrom(0, 0, outputDataL, 0, 0, numSamplesToCopy);
        //buffer.copyFrom(1, 0, outputDataR, 0, 0, numSamplesToCopy);

        // Step 1: Add the first half of the IFFT result to the overlap from the previous block
        for (int i = 0; i < overlapSize; ++i)
        {
            // Add the saved overlap to the first half of the new block
            float currentSampleL = buffer.getSample(0, i) + overlapBuffer.getSample(0, i);
            float currentSampleR = buffer.getSample(1, i) + overlapBuffer.getSample(0, i);

            // Set the final value back to the buffer
            buffer.setSample(0, i, currentSampleL);
            //buffer.setSample(1, i, currentSampleR);
        }

        // Step 2: Copy the second half of the IFFT result into the overlap buffer for the next block
        for (int i = 0; i < overlapSize; ++i)
        {
            overlapBuffer.setSample(0, i, outputDataL.getSample(0, overlapSize + i));
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
