/*
  ==============================================================================

    ChannelSplitter.cpp
    Created: 6 Jun 2020 12:11:27pm
    Author:  maxime

  ==============================================================================
*/

#include "ChannelSplitter.h"

ChannelSplitterProcessor::ChannelSplitterProcessor()
        : BaseEffect("Channel Splitter")
        , paramNumInputs(new AudioParameterChoice("Num Input Channels", "numInChans", {"1", "2"}, 1))
        , paramNumOutputs(new AudioParameterChoice("Num Output Channels", "numOutChans", {"1","2"}, 1))
{
    setLayout(1,1);

    addParameter(paramNumInputs);
    addParameter(paramNumOutputs);

    addRefreshParameterFunction([=] {
        auto newNumInputs = paramNumInputs->getIndex() + 1;
        auto newNumOutputs = paramNumOutputs->getIndex() + 1;

        numInputChannels = newNumInputs;
        numOutputChannels = newNumOutputs;
    });
    setRefreshRate(1);
}

ChannelSplitterProcessor::~ChannelSplitterProcessor() {

}

void ChannelSplitterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {

}

void ChannelSplitterProcessor::releaseResources() {

}

void ChannelSplitterProcessor::processBlock(AudioBuffer<float> &buffer, MidiBuffer &) {
    if (! *bypass) {
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

bool ChannelSplitterProcessor::isBusesLayoutSupported(const AudioProcessor::BusesLayout &layouts) const {
    return AudioProcessor::isBusesLayoutSupported(layouts);
}

