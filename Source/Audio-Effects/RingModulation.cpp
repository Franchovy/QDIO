/*
  ==============================================================================

    This code is based on the contents of the book: "Audio Effects: Theory,
    Implementation and Application" by Joshua D. Reiss and Andrew P. McPherson.

    Code by Juan Gil <https://juangil.com/>.
    Copyright (C) 2017-2019 Juan Gil.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/

#include "RingModulation.h"

//==============================================================================

RingModulationAudioProcessor::RingModulationAudioProcessor():
    BaseEffect("Ring Modulation")
    , paramDepth (new AudioParameterFloat("Depth", "depth",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f))
    , paramFrequency (new AudioParameterFloat("Carrier frequency", "carrierfreq",
            NormalisableRange<float>(10.0f, 1000.0f, 1.0f), 200.0f, "Hz"))
    , paramWaveform (new AudioParameterChoice("Carrier waveform", "carrierwaveform", waveformItemsUI, waveformSine))
{
    setLayout(1,1);

    addParameter(paramDepth);
    addParameter(paramFrequency);
    addParameter(paramWaveform);
}

RingModulationAudioProcessor::~RingModulationAudioProcessor()
{
}

//==============================================================================

void RingModulationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
   /* paramDepth.reset (sampleRate, smoothTime);
    paramFrequency.reset (sampleRate, smoothTime);
    paramWaveform.reset (sampleRate, smoothTime);*/

    //======================================

    lfoPhase = 0.0f;
    inverseSampleRate = 1.0f / (float)sampleRate;
    twoPi = 2.0f * M_PI;
}

void RingModulationAudioProcessor::releaseResources()
{
}

void RingModulationAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    float currentDepth = *paramDepth;
    float currentFrequency = *paramFrequency;
    float phase;

    for (int channel = 0; channel < numInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer (channel);
        phase = lfoPhase;

        for (int sample = 0; sample < numSamples; ++sample) {
            const float in = channelData[sample];
            float carrier = 2.0f * lfo (phase, (int) *paramWaveform) - 1.0f;
            float out = in * (1 - currentDepth + currentDepth * carrier);

            channelData[sample] = out;

            phase += currentFrequency * inverseSampleRate;
            if (phase >= 1.0f)
                phase -= 1.0f;
        }
    }

    lfoPhase = phase;

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================

float RingModulationAudioProcessor::lfo (float phase, int waveform)
{
    float out = 0.0f;

    switch (waveform) {
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
        case waveformSquare: {
            if (phase < 0.5f)
                out = 0.0f;
            else
                out = 1.0f;
            break;
        }
        case waveformSquareSlopedEdges: {
            if (phase < 0.48f)
                out = 1.0f;
            else if (phase < 0.5f)
                out = 1.0f - 50.0f * (phase - 0.48f);
            else if (phase < 0.98f)
                out = 0.0f;
            else
                out = 50.0f * (phase - 0.98f);
            break;
        }
    }

    return out;
}

//==============================================================================




