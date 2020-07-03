/*
  ==============================================================================

    ParameterMod.cpp
    Created: 3 Jul 2020 7:27:30pm
    Author:  maxime

  ==============================================================================
*/

#include "ParameterMod.h"

ParameterMod::ParameterMod()
    : BaseEffect("Parameter Modder")
    , in1(new AudioParameterFloat("Input 1", "input1"
        , NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f))
    , in2(new AudioParameterFloat("Input 2", "input2"
        , NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f))
    , operation(new AudioParameterChoice("Operation", "operation", waveformItemsUI, add))
    , multiplier(new AudioParameterFloat("Multiplier", "multiplier"
        , NormalisableRange<float>(-10.0f, 10.0f, 0.01f), 1.0f))
    , multiplyInputs(new AudioParameterBool("Multiply Inputs", "multiplyins", false))
    , output(new AudioParameterFloat("Output", "output"
        , NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f))
{
    addParameter(in1);
    addParameter(in2);
    addParameter(operation);
    addParameter(multiplier);
    addOutputParameter(output);

    addParameterListener(operation);

    setLayout(0, 0);
}

void ParameterMod::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {

}

void ParameterMod::releaseResources() {

}

void ParameterMod::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) {

    float inputMultiply = *multiplyInputs ? *multiplier : 1.0f;
    float outputMultiply = *multiplyInputs ? 1.0f : *multiplier;

    if (*operation == divide && *in2 == 0.0f) {
        *output = 1.0f;
        return;
    } else {
        switch (*operation) {
            case add:
                *output = (*in1 * inputMultiply + *in2 * inputMultiply) * outputMultiply;
                break;
            case subtract:
                *output = (*in1 * inputMultiply - *in2 * inputMultiply) * outputMultiply;
                break;
            case multiply:
                *output = (*in1 * inputMultiply * *in2 * inputMultiply) * outputMultiply;
                break;
            case divide:
                *output = (*in1 * inputMultiply / *in2 * inputMultiply) * outputMultiply;
                break;
        }
    }


}

void ParameterMod::parameterValueChanged(int parameterIndex, float newValue) {
    if (parameterIndex == operation->getParameterIndex()) {
        switch (*operation) {
            case add:
                *in2 = 0;
                break;
            case subtract:
                *in2 = 0;
                break;
            case multiply:
                *in2 = 1;
                break;
            case divide:
                *in2 = 1;
                break;
        }
    }
}
