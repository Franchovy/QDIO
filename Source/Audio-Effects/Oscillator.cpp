/*
  ==============================================================================

    Oscillator.cpp
    Created: 28 Jun 2020 10:22:45am
    Author:  maxime

  ==============================================================================
*/

#include "Oscillator.h"

Oscillator::Oscillator()
    : BaseEffect()
    , frequency(new AudioParameterFloat("Frequency", "frequency",
            NormalisableRange<float>(0.01f, 10.f, 0.01f), 10.0f, " Hz"))
    , lfoOutput(new AudioParameterFloat("Output", "output",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f))
    , waveform(new AudioParameterChoice("Waveform Type", "waveform", waveformItemsUI, waveformSine))
{
    setLayout(0,0);

    addParameter(frequency);
    addOutputParameter(lfoOutput);
    addParameter(waveform);

    addRefreshParameterFunction([=] {
        std::cout << "frequency: " << *frequency << newLine;
        lfoOutput->setValueNotifyingHost(lfo(lfoPhase));
    });

    setRefreshRate(200);
}

void Oscillator::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    lfoPhase = 0.0f;
    inverseSampleRate = 1.0f / (float)sampleRate;
    twoPi = 2.0f * M_PI;
}

void Oscillator::releaseResources() {

}

void Oscillator::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) {
    float currentFrequency = *frequency;

    float phase = lfoPhase;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        phase += currentFrequency * inverseSampleRate;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }

    lfoPhase = phase;

}

float Oscillator::lfo (float phase)
{
    float out = 0.0f;

    switch (*waveform) {
        case waveformSine: {
            out = 0.5f + 0.5f * sinf (twoPi * phase);
            break;
        }
        case waveformTriangle: {
            if (phase < 0.25f)
                out = 0.5f + 2.0f * phase;
            else if (phase < 0.75f)
                out = 1.0f - 2.0f * (phase - 0.25f);
            else
                out = 2.0f * (phase - 0.75f);
            break;
        }
        case waveformSquare: {
            if (phase < 0.5f)
                out = 1.0f;
            else
                out = 0.0f;
            break;
        }
        case waveformSawtooth: {
            if (phase < 0.5f)
                out = 0.5f + phase;
            else
                out = phase - 0.5f;
            break;
        }
        case waveformInverseSawtooth: {
            if (phase < 0.5f)
                out = 0.5f - phase;
            else
                out = 1.5f - phase;
            break;
        }
    }

    return out;
}