/*
  ==============================================================================

    ChannelSplitter.h
    Created: 6 Jun 2020 12:11:27pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Source/BaseEffects.h"

//==============================================================================

class ChannelSplitterProcessor : public BaseEffect
{
public:
    //==============================================================================

    ChannelSplitterProcessor();
    ~ChannelSplitterProcessor();

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float> &buffer, MidiBuffer&) override;
    //==============================================================================

    AudioParameterChoice* paramNumInputs;
    AudioParameterChoice* paramNumOutputs;

private:
    int numInputChannels;
    int numOutputChannels;
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelSplitterProcessor)
};
