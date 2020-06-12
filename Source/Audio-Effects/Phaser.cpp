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

#include "Phaser.h"

//==============================================================================

PhaserAudioProcessor::PhaserAudioProcessor():
    BaseEffect()
    , paramDepth (new AudioParameterFloat("Depth", "depth",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f))
    , paramFeedback (new AudioParameterFloat("Feedback", "feedback",
            NormalisableRange<float>(0.0f, 0.9f, 0.01f), 0.7f))
    , paramNumFilters (new AudioParameterChoice("Number of filters", "numfilters", {"2", "4", "6", "8", "10"}, 1))
                       //[this](float value){ return paramNumFilters.items[(int)value].getFloatValue(); })
    , paramMinFrequency (new AudioParameterFloat("Min. Frequency", "minfreq",
            NormalisableRange<float>(50.0f, 1000.0f, 1.0f), 80.0f))
    , paramSweepWidth (new AudioParameterFloat("Sweep width", "sweepwidth",
            NormalisableRange<float>(50.0f, 3000.0f, 1.0f), 1000.0f, "Hz"))
    , paramLFOfrequency (new AudioParameterFloat("LFO Frequency", "lfofreq",
            NormalisableRange<float>(0.0f, 2.0f, 0.1f), 0.05f, "Hz"))
    , paramLFOwaveform (new AudioParameterChoice("LFO Waveform", "lfowaveform", waveformItemsUI, waveformSine))
    , paramStereo (new AudioParameterBool("Stereo", "stereo", true))
{
    setLayout(1,1);

    addParameter(paramDepth);
    addParameter(paramFeedback);
    addParameter(paramNumFilters);
    addParameter(paramMinFrequency);
    addParameter(paramSweepWidth);
    addParameter(paramLFOfrequency);
    addParameter(paramLFOwaveform);
    addParameter(paramStereo);
}

PhaserAudioProcessor::~PhaserAudioProcessor()
{
}

//==============================================================================

void PhaserAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramDepth.reset (sampleRate, smoothTime);
    paramFeedback.reset (sampleRate, smoothTime);
    paramNumFilters.reset (sampleRate, smoothTime);
    paramMinFrequency.reset (sampleRate, smoothTime);
    paramSweepWidth.reset (sampleRate, smoothTime);
    paramLFOfrequency.reset (sampleRate, smoothTime);
    paramLFOwaveform.reset (sampleRate, smoothTime);
    paramStereo.reset (sampleRate, smoothTime);
*/
    //======================================

    numFiltersPerChannel = 2 ^ (paramNumFilters->getIndex() + 1);//.items[(int)value].getFloatValue();paramNumFilters.callback (paramNumFilters.items.size() - 1);

    filters.clear();
    for (int i = 0; i < getTotalNumInputChannels() * numFiltersPerChannel; ++i) {
        Filter* filter;
        filters.add (filter = new Filter());
    }

    filteredOutputs.clear();
    for (int i = 0; i < getTotalNumInputChannels(); ++i)
        filteredOutputs.add (0.0f);

    sampleCountToUpdateFilters = 0;
    updateFiltersInterval = 32;

    lfoPhase = 0.0f;
    inverseSampleRate = 1.0f / (float)sampleRate;
    twoPi = 2.0f * M_PI;
}

void PhaserAudioProcessor::releaseResources()
{
}

void PhaserAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    float phase;
    float phaseMain;
    unsigned int sampleCount;

    for (int channel = 0; channel < numInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer (channel);
        sampleCount = sampleCountToUpdateFilters;
        phase = lfoPhase;
        if ((bool)*paramStereo && channel != 0)
            phase = fmodf (phase + 0.25f, 1.0f);

        for (int sample = 0; sample < numSamples; ++sample) {
            float in = channelData[sample];

            float centreFrequency = lfo (phase, (int) *paramLFOwaveform);
            centreFrequency *= *paramSweepWidth;
            centreFrequency += *paramMinFrequency;

            phase += *paramLFOfrequency * inverseSampleRate;
            if (phase >= 1.0f)
                phase -= 1.0f;

            if (sampleCount++ % updateFiltersInterval == 0)
                updateFilters (centreFrequency);

            float filtered = in + *paramFeedback * filteredOutputs[channel];
            for (int i = 0; i < *paramNumFilters; ++i)
                filtered = filters[channel * *paramNumFilters + i]->processSingleSampleRaw (filtered);

            filteredOutputs.set (channel, filtered);
            float out = in + *paramDepth * (filtered - in) * 0.5f;
            channelData[sample] = out;
        }

        if (channel == 0)
            phaseMain = phase;
    }

    sampleCountToUpdateFilters = sampleCount;
    lfoPhase = phaseMain;

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================

void PhaserAudioProcessor::updateFilters (double centreFrequency)
{
    double discreteFrequency = twoPi * centreFrequency * inverseSampleRate;

    for (int i = 0; i < filters.size(); ++i)
        filters[i]->updateCoefficients (discreteFrequency);
}

//==============================================================================

float PhaserAudioProcessor::lfo (float phase, int waveform)
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
    }

    return out;
}

//==============================================================================



