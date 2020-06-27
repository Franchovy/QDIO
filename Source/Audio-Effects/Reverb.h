/*
  ==============================================================================

    DSPEffects.h
    Created: 4 May 2020 11:21:32am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Source/BaseEffects.h"

class ReverbEffect : public BaseEffect
{
public:
    ReverbEffect();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

private:
    AudioParameterFloat* width;
    AudioParameterFloat* damping;
    AudioParameterFloat* dryLevel;
    AudioParameterFloat* wetLevel;
    AudioParameterFloat* roomSize;
    AudioParameterBool* freezeMode;
    AudioParameterBool* stereo;

    Reverb::Parameters reverbParameters;
    Reverb reverbProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbEffect)
};