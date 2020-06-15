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
{
    addParameter(paramLevel);

    setLayout(1,0);

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
        recordedLevel = (recordedLevel + buffer.getRMSLevel(c, 0, buffer.getNumSamples())) / 2;
    }
}
