/*
  ==============================================================================

    ChannelSplitter.cpp
    Created: 6 Jun 2020 12:11:27pm
    Author:  maxime

  ==============================================================================
*/

#include "ChannelSplitter.h"

ChannelSplitterAudioProcessor::ChannelSplitterAudioProcessor()
        : BaseEffect("Channel Splitter")
        , paramNumInputs(new AudioParameterChoice("Num Input Channels", "numInChans", {"1", "2"}, 1))
        , paramNumOutputs(new AudioParameterChoice("Num Output Channels", "numOutChans", {"1","2"}, 1))
{
    setLayout(1,1);

    addParameter(paramNumInputs);
    addParameter(paramNumOutputs);

}

ChannelSplitterAudioProcessor::~ChannelSplitterAudioProcessor() {

}

void ChannelSplitterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {

}

void ChannelSplitterAudioProcessor::releaseResources() {

}

void ChannelSplitterAudioProcessor::processBlock(AudioBuffer<float> &buffer, MidiBuffer &) {
    if (! *bypass) {
        int numInputChannels = paramNumOutputs->getIndex() + 1;
        int numOutputChannels = paramNumInputs->getIndex() + 1;

        // Make sure there are enough channels.
        if (getTotalNumInputChannels() < numInputChannels
            || getTotalNumOutputChannels() < numOutputChannels)
        {
            return;
        }

        int numSamples = buffer.getNumSamples();

        if (numOutputChannels == 1) { // Mono
            // Apply gain to keep level
            float gain = 1.0f / (float) numInputChannels;
            buffer.applyGain(gain);

            for (int c = 1; c < numInputChannels; c++) {
                // Put all channels into buffer
                buffer.addFrom(0, 0, buffer, c, 0, numSamples);
            }
        } else if (numOutputChannels == 2) { // Stereo

            buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
            /*for (int c = 1; c < numOutputChannels; c++) {
                buffer.copyFrom(c, 0, buffer, c % numInputChannels, 0, numSamples);
            }*/
        }
    }
}



