/*
  ==============================================================================

    Level.cpp
    Created: 15 Jun 2020 4:45:45pm
    Author:  maxime

  ==============================================================================
*/

#include "Level.h"

LevelAudioProcessor::LevelAudioProcessor() :
        BaseEffect()
    , paramLevel(new AudioParameterFloat("Level", "level",
                                         NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f))
    , paramDecay(new AudioParameterFloat("Decay Speed", "decay",
                                         NormalisableRange<float>(1.0f, 1.2f, 0.01f), 1.05f))
{
    addParameter(paramLevel);
    addParameter(paramDecay);

    setLayout(1,1);

    addRefreshParameterFunction([=] {
        paramLevel->setValueNotifyingHost(recordedLevel);
    });

    setRefreshRate(60);
}

void LevelAudioProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {

}

void LevelAudioProcessor::releaseResources() {

}

void LevelAudioProcessor::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) {
    for (int c = 0; c < buffer.getNumChannels(); c++) {
        recordedLevel = jmax((float) recordedLevel, buffer.getRMSLevel(c, 0, buffer.getNumSamples()));
    }
}

void LevelAudioProcessor::timerCallback() {
    recordedLevel = recordedLevel / *paramDecay;

    BaseEffect::timerCallback();
}
