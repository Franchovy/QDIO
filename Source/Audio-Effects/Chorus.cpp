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

#include "Chorus.h"

//==============================================================================

ChorusAudioProcessor::ChorusAudioProcessor():
    BaseEffect ("Chorus")
    , paramDelay (new AudioParameterFloat("Delay", "delay",
            NormalisableRange<float>(10.0f, 50.0f, 0.01f), 30.0f, "ms"))
    , paramWidth (new AudioParameterFloat("Width", "width",
            NormalisableRange<float>(10.0f, 50.0f, 0.01f), 20.0f, "ms"))
    , paramDepth (new AudioParameterFloat("Depth", "depth",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f))
    , paramNumVoices (new AudioParameterChoice("Number of voices", "numVoices",
            {"2", "3", "4", "5"}, 0)) //callback: [](float value){ return value + 2; }
    , paramFrequency (new AudioParameterFloat("LFO Frequency", "lfo_frequency",
            NormalisableRange<float>(0.05f, 2.0f, 0.01f), 0.2f, "Hz"))
    , paramWaveform (new AudioParameterChoice("LFO Waveform", "lfo_waveform",
            waveformItemsUI, waveformSine))
    , paramInterpolation (new AudioParameterChoice("Interpolation", "interpolation",
            interpolationItemsUI, interpolationLinear))
    , paramStereo (new AudioParameterBool("Stereo", "stereo", true))
{
    setLayout(1,1);

    addParameter(paramDelay);
    addParameter(paramWidth);
    addParameter(paramDepth);
    addParameter(paramNumVoices);
    addParameter(paramFrequency);
    addParameter(paramWaveform);
    addParameter(paramInterpolation);
    addParameter(paramStereo);
}

ChorusAudioProcessor::~ChorusAudioProcessor()
{
}

//==============================================================================

void ChorusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramDelay.reset (sampleRate, smoothTime);
    paramWidth.reset (sampleRate, smoothTime);
    paramDepth.reset (sampleRate, smoothTime);
    paramNumVoices.reset (sampleRate, smoothTime);
    paramFrequency.reset (sampleRate, smoothTime);
    paramWaveform.reset (sampleRate, smoothTime);
    paramInterpolation.reset (sampleRate, smoothTime);
    paramStereo.reset (sampleRate, smoothTime);
*/
    //======================================

    float maxDelayTime = paramDelay->getNormalisableRange().end + paramWidth->getNormalisableRange().end;
    delayBufferSamples = (int)(maxDelayTime * (float)sampleRate) + 1;
    if (delayBufferSamples < 1)
        delayBufferSamples = 1;

    delayBufferChannels = getTotalNumInputChannels();
    delayBuffer.setSize (delayBufferChannels, delayBufferSamples);
    delayBuffer.clear();

    delayWritePosition = 0;
    lfoPhase = 0.0f;
    inverseSampleRate = 1.0f / (float)sampleRate;
    twoPi = 2.0f * M_PI;
}

void ChorusAudioProcessor::releaseResources()
{
}

void ChorusAudioProcessor::processBlock (AudioBuffer<float> &buffer, MidiBuffer& midiMessages)
{
    if (! *bypass) {
        ScopedNoDenormals noDenormals;

        const int numInputChannels = getTotalNumInputChannels();
        const int numOutputChannels = getTotalNumOutputChannels();
        const int numSamples = buffer.getNumSamples();

        //======================================

        float currentDelay = *paramDelay * 0.001f;
        float currentWidth = *paramWidth * 0.001f;
        float currentDepth = *paramDepth;
        int numVoices = paramNumVoices->getIndex() + 2;
        float currentFrequency = *paramFrequency;
        bool stereo = (bool) *paramStereo;

        int localWritePosition;
        float phase;

        for (int channel = 0; channel < numInputChannels; ++channel) {
            float *channelData = buffer.getWritePointer(channel);
            float *delayData = delayBuffer.getWritePointer(channel);
            localWritePosition = delayWritePosition;
            phase = lfoPhase;

            for (int sample = 0; sample < numSamples; ++sample) {
                const float in = channelData[sample];
                float out = 0.0f;
                float phaseOffset = 0.0f;
                float weight;

                for (int voice = 0; voice < numVoices - 1; ++voice) {
                    if (stereo && numVoices > 2) {
                        weight = (float) voice / (float) (numVoices - 2);
                        if (channel != 0)
                            weight = 1.0f - weight;
                    } else {
                        weight = 1.0f;
                    }

                    float localDelayTime =
                            (currentDelay + currentWidth * lfo(phase + phaseOffset, (int) *paramWaveform)) *
                            (float) getSampleRate();

                    float readPosition =
                            fmodf((float) localWritePosition - localDelayTime + (float) delayBufferSamples,
                                  delayBufferSamples);
                    int localReadPosition = floorf(readPosition);

                    switch ((int) *paramInterpolation) {
                        case interpolationNearestNeighbour: {
                            float closestSample = delayData[localReadPosition % delayBufferSamples];
                            out = closestSample;
                            break;
                        }
                        case interpolationLinear: {
                            float fraction = readPosition - (float) localReadPosition;
                            float delayed0 = delayData[(localReadPosition + 0)];
                            float delayed1 = delayData[(localReadPosition + 1) % delayBufferSamples];
                            out = delayed0 + fraction * (delayed1 - delayed0);
                            break;
                        }
                        case interpolationCubic: {
                            float fraction = readPosition - (float) localReadPosition;
                            float fractionSqrt = fraction * fraction;
                            float fractionCube = fractionSqrt * fraction;

                            float sample0 = delayData[(localReadPosition - 1 + delayBufferSamples) %
                                                      delayBufferSamples];
                            float sample1 = delayData[(localReadPosition + 0)];
                            float sample2 = delayData[(localReadPosition + 1) % delayBufferSamples];
                            float sample3 = delayData[(localReadPosition + 2) % delayBufferSamples];

                            float a0 = -0.5f * sample0 + 1.5f * sample1 - 1.5f * sample2 + 0.5f * sample3;
                            float a1 = sample0 - 2.5f * sample1 + 2.0f * sample2 - 0.5f * sample3;
                            float a2 = -0.5f * sample0 + 0.5f * sample2;
                            float a3 = sample1;
                            out = a0 * fractionCube + a1 * fractionSqrt + a2 * fraction + a3;
                            break;
                        }
                    }

                    if (stereo && numVoices == 2)
                        channelData[sample] = (channel == 0) ? in : out * currentDepth;
                    else
                        channelData[sample] += out * currentDepth * weight;

                    if (numVoices == 3)
                        phaseOffset += 0.25f;
                    else if (numVoices > 3)
                        phaseOffset += 1.0f / (float) (numVoices - 1);
                }

                delayData[localWritePosition] = in;

                if (++localWritePosition >= delayBufferSamples)
                    localWritePosition -= delayBufferSamples;

                phase += currentFrequency * inverseSampleRate;
                if (phase >= 1.0f)
                    phase -= 1.0f;
            }
        }

        delayWritePosition = localWritePosition;
        lfoPhase = phase;

        //======================================

        for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
            buffer.clear(channel, 0, numSamples);
    }
}

//==============================================================================

float ChorusAudioProcessor::lfo (float phase, int waveform)
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
    }

    return out;
}

//==============================================================================






//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
