#include "FFTProcessor.h"
#include <math.h>
#include <algorithm>
#include <iterator>

//FFTProcessor::FFTProcessor() : fft(8), fourierf(8), fourieri(8), window(256, juce::dsp::WindowingFunction<float>::blackmanHarris) // fftSize = 2^fftOrder



// Prepare function (optional)
void FFTProcessor::prepare(double sampleRate, int samplesPerBlock)
{
    //// This can be used to allocate additional resources or set up specific parameters
    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;
}

// Process the left and right channels
void FFTProcessor::processFFT(juce::AudioBuffer<float> bufferinput, int channel)
{
    // Get the audio blocks for left and right channels
    //auto& block = cntxt.getOutputBlock();

    //juce::ScopedNoDenormals noDenormals;

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
   
        

    // Process the left channel with FFT
    /*applyFFT(block, fftData);*/
    applyFFT(bufferinput, channel);

    // Perform any frequency domain processing here (both fftDataLeft and fftDataRight)

    // Inverse FFT to convert back to time domain for left and right channels
    //applyInverseFFT(block, fftData);
}

void FFTProcessor::processIFFT(float* m, float* p, int channel)
{
    // Get the audio blocks for left and right channels
    //auto& block = cntxt.getOutputBlock();


    // Process the left channel with FFT

    applyInverseFFT(m, p, channel);
    /*applyInverseFFT(block, fftData);*/

    // Perform any frequency domain processing here (both fftDataLeft and fftDataRight)

    // Inverse FFT to convert back to time domain for left and right channels
    //applyInverseFFT(block, fftData);
}

void FFTProcessor::storeFrequencyData(const juce::AudioBuffer<float>& fftData)
{
    frequencyData.setSize(1, fftsize);
    // Store magnitude of FFT data into frequencyData buffer (can be modified to store real/imaginary as needed)
    for (int i = 0; i < fftsize / 2; ++i)
    {
        // For real FFT, even indices are real, odd indices are imaginary
        float realPart = fftData.getSample(0, 2 * i);    // Real part
        float imagPart = fftData.getSample(0, 2 * i + 1); // Imaginary part

        // Store magnitude (or any other transformation you need)
        float magnitude = std::sqrt(realPart * realPart + imagPart * imagPart) / fftsize;
        frequencyData.setSample(0, i, magnitude);

        DBG("freqData " << i << frequencyData.getSample(0, i));
    }
}


    // Function to apply FFT to a single channel (left or right)
//void FFTProcessor::applyFFT(juce::dsp::AudioBlock<float>& audioBlock, juce::AudioBuffer<float>& fftData)
//{
//    const float* inputSamples = audioBlock.getChannelPointer(0);
//
//    // Copy input samples into FFT buffer (zero-pad if necessary)
//    for (int i = 0; i < fftSize; ++i)
//    {
//        fftData.setSample(0, i, i < audioBlock.getNumSamples() ? inputSamples[i] : 0.0f);
//    }
//
//    // Perform forward FFT
//    fft.performRealOnlyForwardTransform(fftData.getWritePointer(0));
//
//    for (int i = 0; i < fftSize; i++) {
//        std::complex<float> temp;
//        temp.real(fftData.getWritePointer(0));
//        temp.imag(fftData.getWritePointer(0))
//    }
//
//    DBG("FFT Output Data:");
//    for (int i = 0; i < fftSize; ++i)
//    {
//        if (i < 10)  // Again, limit to the first 10 values for easier viewing
//            DBG("FFT Data " << i << ": " << fftData.getSample(0, i));
//    }
//
//    storeFrequencyData(fftData);
//}

void FFTProcessor::applyFFT(juce::AudioBuffer<float> bufferinput, int channel) {

    auto* channelData = bufferinput.getWritePointer(0);

    for (int x = 0; x < bufferinput.getNumSamples(); x++) {
        float f = channelData[x];

        int writehead = selectWritehead(channel);
        int playhead = selectPlayhead(channel);
        int windowLoc = selectWindowLoc(channel);
        float* bufout = selectBufOut(channel);
        float* btp = selectBTP(channel);

        channelData[x] = bufout[writehead];
        btp[playhead] = f;

        updateWritehead(channel, 0);
        updatePlayhead(channel, 0);
        updateWindowLoc(channel);
        writehead = selectWritehead(channel);
        playhead = selectPlayhead(channel);
        windowLoc = selectWindowLoc(channel);

        if (windowLoc == fftsize / 4) {

            
            windowLoc = 0;
            int counter = 0;
            for (int i = playhead; i < fftsize; i++) {
                segmented[counter] = btp[i];
                counter++;
            }


            for (int i = 0; i < playhead; i++) {
                segmented[counter] = btp[i];
                counter++;
            }

            window.multiplyWithWindowingTable(segmented, fftsize);
            fourierf.performRealOnlyForwardTransform(segmented);
            for (int i = 0; i < fftsize; i++) {
                std::complex<float> temp;
                temp.real(segmented[i * 2]);
                temp.imag(segmented[i * 2 + 1]);

                mag[i] = std::abs(temp);
                phase[i] = std::arg(temp);
            }
        }
    }
}

// Function to apply inverse FFT and copy the result back to the audio buffer
void FFTProcessor::applyInverseFFT(float* m, float* p, int channel)
{

    int writehead = selectWritehead(channel);
    int playhead = selectPlayhead(channel);
    int windowLoc = selectWindowLoc(channel);
    float* bufout = selectBufOut(channel);
    float* btp = selectBTP(channel);

    if (windowLoc == fftsize / 4) {
        for (int i = 0; i < fftsize; i++) {
            float real = std::cos(p[i]) * m[i];
            float imag = std::sin(p[i]) * m[i];


            if (i < 25) {
                DBG("segBefore " << i << ": " << segmented[i]);
                DBG("m " << i << ": " << m[i]);
                DBG("p " << i << ": " << p[i]);
                DBG("real " << i << ": " << real);
                DBG("imag " << i << ": " << imag);
            }

            segmented[i * 2] = real;
            segmented[i * 2 + 1] = imag;


        }

        for (int i = 0; i < 25; i++) {
            
            DBG("segmented " << i << ": " << segmented[i]);
        }

        fourieri.performRealOnlyInverseTransform(segmented);
        window.multiplyWithWindowingTable(segmented, fftsize);

        


        for (int i = 0; i < fftsize; i++) {
            if (i < (fftsize / 4) * 3) { // 75% overlap
                bufout[(writehead + i) % fftsize] += segmented[i] * (2.f / 3.f);
                //bufout[(writehead + i) % fftsize] += segmented[i];
            }
            else {
                bufout[(writehead + i) % fftsize] = segmented[i] * (2.f / 3.f);
                //bufout[(writehead + i) % fftsize] = segmented[i];
            }
        }

        for (int i = 0; i < 200; i++) {
            DBG("bufout " << i << ": " << bufout[i]);
        }
    }

    updatePlayhead(channel, 1);
    updateWritehead(channel, 1);

}

float* FFTProcessor::selectBufOut(int channel)
{
    if (channel == 0) return leftbufout;
    if (channel == 1) return rightbufout;
    if (channel == 2) return leftauxbufout;
    if (channel == 3) return rightauxbufout;

    return nullptr;  // Handle invalid channels
}

float* FFTProcessor::selectBTP(int channel)
{

    if (channel == 0) return leftbuf;
    if (channel == 1) return rightbuf;
    if (channel == 2) return leftauxbuf;
    if (channel == 3) return rightauxbuf;
    return nullptr;

}

void FFTProcessor::updatePlayhead(int channel, int after)
{
    if (after == 0)
    {
        if (channel == 0) playheadL++;

        if (channel == 1)  playheadR++;

        if (channel == 2) playheadLA++;

        if (channel == 3) playheadRA++;
    }


    if (after == 1) {

        if (channel == 0)
        {
            
            if (playheadL == fftsize) {
                playheadL = 0;

            }
        }

        if (channel == 1)
        {
            
            if (playheadR == fftsize) {
                playheadR = 0;

            }
        }

        if (channel == 2)
        {
            
            if (playheadLA == fftsize) {
                playheadLA = 0;

            }
        }

        if (channel == 3)
        {
            
            if (playheadRA == fftsize) {
                playheadRA = 0;

            }
        }

    }
}


int FFTProcessor::selectPlayhead(int channel)
{
    if (channel == 0) return playheadL;
    if (channel == 1) return playheadR;
    if (channel == 2) return playheadLA;
    if (channel == 3) return playheadRA;
}

void FFTProcessor::updateWritehead(int channel, int after)
{
    if (after == 0) {
        if (channel == 0)
        {
            writeheadL++;
        }

        if (channel == 1)
        {
            writeheadR++;
        }

        if (channel == 2)
        {
            writeheadLA++;
        }

        if (channel == 3)
        {
            writeheadRA++;
        }
    }

    if (after == 1) {

        if (channel == 0)
        {
            if (writeheadL == fftsize) {
                writeheadL = 0;
            }
        }

        if (channel == 1)
        {
            if (writeheadR == fftsize) {
                writeheadR = 0;
            }
        }

        if (channel == 2)
        {
            if (writeheadLA == fftsize) {
                writeheadLA = 0;
            }
        }

        if (channel == 3)
        {
            if (writeheadRA == fftsize) {
                writeheadRA = 0;
            }
        }
        
    }
}

int FFTProcessor::selectWritehead(int channel)
{
    if (channel == 0)
    {
        return writeheadL;
    }

    if (channel == 1)
    {
        return writeheadR;
    }

    if (channel == 2)
    {
        return writeheadLA;
    }

    if (channel == 3)
    {
        return writeheadRA;
    }
}

void FFTProcessor::updateWindowLoc(int channel)
{
    
        if (channel == 0)
        {
            if (windowL == fftsize / 4) {
                windowL = 0;
            }
            windowL++;
            
        }

        if (channel == 1)
        {
            if (windowR == fftsize / 4) {
                windowR = 0;
            }
            windowR++;
            
        }

        if (channel == 2)
        {
            if (windowLA == fftsize / 4) {
                windowLA = 0;
            }
            windowLA++;
            
        }

        if (channel == 3)
        {
            if (windowRA == fftsize / 4) {
                windowRA = 0;
            }
            windowRA++;
            
        }

        
    
}

int FFTProcessor::selectWindowLoc(int channel)
{

    if (channel == 0)
    {
        int windowLoc = windowL;
        return windowLoc;
    }

    if (channel == 1)
    {
        int windowLoc = windowR;
        return windowLoc;
    }

    if (channel == 2)
    {
        int windowLoc = windowLA;
        return windowLoc;
    }

    if (channel == 3)
    {
        int windowLoc = windowRA;
        return windowLoc;
    }
}

float* FFTProcessor::getOutputData(int channel) {
    if (channel == 0)
    {
        return leftbufout;
    }

    if (channel == 1)
    {
        return rightbufout;
    }

    if (channel == 2)
    {
        return leftauxbufout;
    }

    if (channel == 3)
    {
        return rightauxbufout;
    }
}