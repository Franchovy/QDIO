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

#include "CompressorExpander.h"

//==============================================================================

CompressorExpanderAudioProcessor::CompressorExpanderAudioProcessor()
    : BaseEffect("Compressor/Expander")
    , paramMode (new AudioParameterChoice("Mode", "mode", {"Compressor / Limiter", "Expander / Noise gate"}, 1))
    , paramThreshold (new AudioParameterFloat("Threshold", "threshold",
            NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -24.0f, "dB"))
    , paramRatio (new AudioParameterFloat("Ratio", "ratio",
            NormalisableRange<float>(1.0f, 100.0f, 0.1f), 50.0f, ":1"))
    , paramAttack (new AudioParameterFloat("Attack", "attack",
            NormalisableRange<float>(0.1f, 100.0f, 0.1f), 2.0f, "ms")) //[](float value){ return value * 0.001f; })
    , paramRelease (new AudioParameterFloat("Release", "release",
            NormalisableRange<float>(10.0f, 1000.0f, 0.1f), 300.0f, "ms"))// [](float value){ return value * 0.001f; })
    , paramMakeupGain (new AudioParameterFloat("Makeup gain", "makeupGain",
            NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.01f, "dB"))
{
    setLayout(1,1);

    addParameter(paramMode);
    addParameter(paramThreshold);
    addParameter(paramRatio);
    addParameter(paramAttack);
    addParameter(paramRelease);
    addParameter(paramMakeupGain);
}

CompressorExpanderAudioProcessor::~CompressorExpanderAudioProcessor()
{
}

//==============================================================================

void CompressorExpanderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramThreshold.reset (sampleRate, smoothTime);
    paramRatio.reset (sampleRate, smoothTime);
    paramAttack.reset (sampleRate, smoothTime);
    paramRelease.reset (sampleRate, smoothTime);
    paramMakeupGain.reset (sampleRate, smoothTime);
    paramBypass.reset (sampleRate, smoothTime);*/

    //======================================

    mixedDownInput.setSize (1, samplesPerBlock);

    inputLevel = 0.0f;
    ylPrev = 0.0f;

    inverseSampleRate = 1.0f / (float)getSampleRate();
    inverseE = 1.0f / M_E;
}

void CompressorExpanderAudioProcessor::releaseResources()
{
}

void CompressorExpanderAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    if (! *bypass) {
        ScopedNoDenormals noDenormals;

        const int numInputChannels = getTotalNumInputChannels();
        const int numOutputChannels = getTotalNumOutputChannels();
        const int numSamples = buffer.getNumSamples();

        //======================================

        if ((bool) *bypass)
            return;

        //======================================

        mixedDownInput.clear();
        for (int channel = 0; channel < numInputChannels; ++channel)
            mixedDownInput.addFrom(0, 0, buffer, channel, 0, numSamples, 1.0f / numInputChannels);

        for (int sample = 0; sample < numSamples; ++sample) {
            bool expander = (bool) *paramMode;
            float T = *paramThreshold;
            float R = *paramRatio;
            float alphaA = calculateAttackOrRelease(*paramAttack * 0.001f);
            float alphaR = calculateAttackOrRelease(*paramRelease * 0.001f);
            float makeupGain = *paramMakeupGain;

            float inputSquared = powf(mixedDownInput.getSample(0, sample), 2.0f);
            if (expander) {
                const float averageFactor = 0.9999f;
                inputLevel = averageFactor * inputLevel + (1.0f - averageFactor) * inputSquared;
            } else {
                inputLevel = inputSquared;
            }
            xg = (inputLevel <= 1e-6f) ? -60.0f : 10.0f * log10f(inputLevel);

            // Expander
            if (expander) {
                if (xg > T)
                    yg = xg;
                else
                    yg = T + (xg - T) * R;

                xl = xg - yg;

                if (xl < ylPrev)
                    yl = alphaA * ylPrev + (1.0f - alphaA) * xl;
                else
                    yl = alphaR * ylPrev + (1.0f - alphaR) * xl;

                // Compressor
            } else {
                if (xg < T)
                    yg = xg;
                else
                    yg = T + (xg - T) / R;

                xl = xg - yg;

                if (xl > ylPrev)
                    yl = alphaA * ylPrev + (1.0f - alphaA) * xl;
                else
                    yl = alphaR * ylPrev + (1.0f - alphaR) * xl;
            }

            control = powf(10.0f, (makeupGain - yl) * 0.05f);
            ylPrev = yl;

            for (int channel = 0; channel < numInputChannels; ++channel) {
                float newValue = buffer.getSample(channel, sample) * control;
                buffer.setSample(channel, sample, newValue);
            }
        }

        //======================================

        for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
            buffer.clear(channel, 0, numSamples);
    }
}

//==============================================================================

float CompressorExpanderAudioProcessor::calculateAttackOrRelease (float value)
{
    if (value == 0.0f)
        return 0.0f;
    else
        return pow (inverseE, inverseSampleRate / value);
}

//==============================================================================






#ifndef JucePlugin_PreferredChannelConfigurations
bool CompressorExpanderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
