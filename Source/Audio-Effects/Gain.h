/*
  ==============================================================================

    Gain.h
    Created: 7 Jun 2020 11:58:11am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Source/BaseEffects.h"

//==============================================================================

class GainAudioProcessor : public BaseEffect
{
public:
    //==============================================================================

    GainAudioProcessor();
    ~GainAudioProcessor();

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    AudioParameterFloat* gain;

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainAudioProcessor)
};
