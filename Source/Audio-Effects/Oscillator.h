/*
  ==============================================================================

    Oscillator.h
    Created: 28 Jun 2020 10:22:45am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../BaseEffects.h"

class Oscillator : public BaseEffect
{
public:
    Oscillator();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;

    void releaseResources() override;

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

    float lfoPhase;
    float inverseSampleRate;
    float twoPi;

    float lfo (float phase);

    StringArray waveformItemsUI = {
            "Sine",
            "Triangle",
            "Square",
            "Sawtooth (rising)",
            "Sawtooth (falling)"
    };

    enum waveformIndex {
        waveformSine = 0,
        waveformTriangle,
        waveformSquare,
        waveformSawtooth,
        waveformInverseSawtooth,
    };

private:
    AudioParameterFloat* frequency;
    AudioParameterFloat* lfoOutput;
    AudioParameterChoice* waveform;
};