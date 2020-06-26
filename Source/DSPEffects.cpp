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
    , roomSize(new AudioParameterFloat("Room Size", "roomsize",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f))
    , width(new AudioParameterFloat("Width", "width",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f))
    , damping(new AudioParameterFloat("Damping", "damping",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f))
    , dryLevel(new AudioParameterFloat("Dry Level", "drylevel",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f))
    , wetLevel(new AudioParameterFloat("Wet Level", "wetlevel",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f))
    , freezeMode(new AudioParameterBool("Freeze Mode", "freezemode", 0.0f))
    , stereo(new AudioParameterBool("Stereo", "stereo", 0.0f))
{
    name = "Reverb";
    addParameter(roomSize);
    addParameter(width);
    addParameter(damping);
    addParameter(dryLevel);
    addParameter(wetLevel);
    addParameter(freezeMode);
    addParameter(stereo);

    addRefreshParameterFunction([=] {
        bool parametersChanged = false;
        if (reverbParameters.width != *width) {
            reverbParameters.width = *width;
            parametersChanged = true;
        }
        if (reverbParameters.damping != *damping) {
            reverbParameters.damping = *damping;
            parametersChanged = true;
        }
        if (reverbParameters.dryLevel != *dryLevel) {
            reverbParameters.dryLevel = *dryLevel;
            parametersChanged = true;
        }
        if (reverbParameters.wetLevel != *wetLevel) {
            reverbParameters.wetLevel = *wetLevel;
            parametersChanged = true;
        }
        if (reverbParameters.roomSize != *roomSize) {
            reverbParameters.roomSize = *roomSize;
            parametersChanged = true;
        }
        if (reverbParameters.freezeMode != (float) *freezeMode) {
            reverbParameters.freezeMode = (float) *freezeMode;
            parametersChanged = true;
        }

        if (parametersChanged) {
            reverbProcessor.setParameters(reverbParameters);
        }
    });
    setRefreshRate(10);
}

void ReverbEffect::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    reverbProcessor.reset();

    reverbParameters.width = *width;
    reverbParameters.damping = *damping;
    reverbParameters.dryLevel = *dryLevel;
    reverbParameters.wetLevel = *wetLevel;
    reverbParameters.roomSize = *roomSize;
    reverbParameters.freezeMode = (float) *freezeMode;

    reverbProcessor.setParameters(reverbParameters);
}

void ReverbEffect::releaseResources() {
    reverbProcessor.reset();
}

void ReverbEffect::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) {
    if (! bypass->get()) {
        if (! *stereo) {
            reverbProcessor.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
        }
        // too dangerous...
        /*else {
            reverbProcessor.processStereo(buffer.getWritePointer(0),
                    buffer.getWritePointer(1), buffer.getNumSamples());
        }*/
    }
    /*for (int c = 0; c < buffer.getNumChannels(); c++) {
        reverbProcessor.processMono(buffer.getWritePointer(c), buffer.getNumSamples());
    }*/
}


