/*
  ==============================================================================

    DSPEffects.cpp
    Created: 4 May 2020 11:21:32am
    Author:  maxime

  ==============================================================================
*/

#include "DSPEffects.h"

DSPEffect::DSPEffect()
    : BaseEffect()
{
    setLayout(1,1);
}


ReverbEffect::ReverbEffect()
    : DSPEffect()
    , roomSize("roomsize", "Room Size",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f)
    , strength("strength", "Strength",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f)
{
    name = "Reverb";
    addParameter(&roomSize);
    //addParameter(&strength);

    startTimer(1000);
}

void ReverbEffect::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    reverbParameters.width = 0.5f;
    reverbParameters.damping = 0.8f;
    reverbParameters.dryLevel = 0.5f;
    reverbParameters.wetLevel = 0.5f;
    reverbParameters.roomSize = 1.0f;
    reverbParameters.freezeMode = 0.0f;

    reverbProcessor.setParameters(reverbParameters);
}

void ReverbEffect::releaseResources() {

}

void ReverbEffect::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) {
    for (int c = 0; c < buffer.getNumChannels(); c++) {
        reverbProcessor.processMono(buffer.getWritePointer(c), buffer.getNumSamples());
    }
}

void ReverbEffect::timerCallback() {
    if (reverbParameters.roomSize != roomSize) {
        reverbParameters.roomSize = roomSize;
        reverbProcessor.setParameters(reverbParameters);
    }
    if (reverbParameters.damping != strength) {
        reverbParameters.damping = strength;
        reverbProcessor.setParameters(reverbParameters);
    }
    startTimer(100);
}


