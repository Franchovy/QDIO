/*
  ==============================================================================

    ParameterMod.h
    Created: 3 Jul 2020 7:27:30pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../BaseEffects.h"

class ParameterMod : public BaseEffect
{
public:
    ParameterMod();

    StringArray waveformItemsUI = {
            "Add",
            "Subtract",
            "Multiply",
            "Divide"
    };

    enum waveformIndex {
        add = 0,
        subtract,
        multiply,
        divide,
    };

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

    void parameterValueChanged(int parameterIndex, float newValue) override;

private:
    AudioParameterFloat* in1;
    AudioParameterFloat* in2;
    AudioParameterChoice* operation;
    AudioParameterFloat* multiplier;
    AudioParameterBool* multiplyInputs;
    AudioParameterFloat* output;
};