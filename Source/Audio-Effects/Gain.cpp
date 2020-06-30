/*
  ==============================================================================

    Gain.cpp
    Created: 7 Jun 2020 11:58:11am
    Author:  maxime

  ==============================================================================
*/

#include "Gain.h"

GainAudioProcessor::GainAudioProcessor()
    : BaseEffect("Gain")
    , gain(new AudioParameterFloat("Gain", "gain", NormalisableRange<float>(0.1f,10.0f,0.01f), 2.0f))
{
    setLayout(1,1);
    addParameter(gain);
}

GainAudioProcessor::~GainAudioProcessor() {

}

void GainAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {

}

void GainAudioProcessor::releaseResources() {

}

void GainAudioProcessor::processBlock(AudioSampleBuffer &buffer, MidiBuffer &) {
    if (! *bypass) {
        buffer.applyGain(*gain);
    }
}
