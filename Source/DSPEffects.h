/*
  ==============================================================================

    DSPEffects.h
    Created: 4 May 2020 11:21:32am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "BaseEffects.h"

class DSPEffect : public BaseEffect
{
public:
    DSPEffect();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSPEffect)
};

class ReverbEffect : public DSPEffect
{
public:
    ReverbEffect();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

private:
    AudioParameterFloat* roomSize;
    AudioParameterFloat* strength;

    Reverb::Parameters reverbParameters;
    Reverb reverbProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbEffect)
};