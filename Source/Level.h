/*
  ==============================================================================

    Level.h
    Created: 15 Jun 2020 4:45:45pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "BaseEffects.h"

class LevelAudioProcessor : public BaseEffect
{
public:
    LevelAudioProcessor();

    void timerCallback() override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;

    void releaseResources() override;

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

    AudioParameterFloat* paramLevel;
    AudioParameterFloat* paramDecay;

private:
    std::atomic<float> recordedLevel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelAudioProcessor);
};