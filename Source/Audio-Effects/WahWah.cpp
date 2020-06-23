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

#include "WahWah.h"

//==============================================================================

WahWahAudioProcessor::WahWahAudioProcessor():
    BaseEffect()
    , paramMode (new AudioParameterChoice("Mode", "mode", modeItemsUI, modeManual))
    , paramMix (new AudioParameterFloat("Mix", "mix", NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f, ""))
    , paramFrequency (new AudioParameterFloat("Frequency", "freq", NormalisableRange<float>(200.f, 1300.0f, 1.0f), 300.f, "Hz"))
                      //[this](float value){ paramFrequency.setCurrentAndTargetValue (value); updateFilters(); return value; })
    , paramQfactor (new AudioParameterFloat("Q Factor", "qfactor", NormalisableRange<float>(0.1f, 20.0f, 0.01f), 10.0f, ""))
                   // [this](float value){ paramQfactor.setCurrentAndTargetValue (value); updateFilters(); return value; })
    , paramGain (new AudioParameterFloat("Gain", "gain", NormalisableRange<float>(0.0f, 20.0f, 0.1f), 20.0f, "dB"))
            //[this](float value){ paramGain.setCurrentAndTargetValue (value); updateFilters(); return value; })
    , paramFilterType (new AudioParameterChoice("Filter type", "filtertype", filterTypeItemsUI, filterTypeResonantLowPass))
                       //[this](float value){ paramFilterType.setCurrentAndTargetValue (value); updateFilters(); return value; })
    , paramLFOfrequency (new AudioParameterFloat("LFO Frequency", "lfofreq", NormalisableRange<float>(0.0f, 5.0f, 0.01f), 2.0f, "Hz"))
    , paramMixLFOandEnvelope (new AudioParameterFloat("LFO/Env", "lfoenv", NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f, ""))
    , paramEnvelopeAttack (new AudioParameterFloat("Env. Attack", "envattack", NormalisableRange<float>(0.1f, 100.0f, 0.1f), 2.0f, "ms"))
                            //[](float value){ return value * 0.001f; })
    , paramEnvelopeRelease (new AudioParameterFloat("Env. Release", "envrelease", NormalisableRange<float>(10.0f, 1000.f, 1.0f), 300.f, "ms"))
                        //[](float value){ return value * 0.001f; })
{
    centreFrequency = *paramFrequency;

    setLayout(1,1);
    addParameter(paramMode);
    addParameter(paramMix);
    addParameter(paramFrequency);
    addParameter(paramQfactor);
    addParameter(paramGain);
    addParameter(paramFilterType);
    addParameter(paramLFOfrequency);
    addParameter(paramMixLFOandEnvelope);
    addParameter(paramEnvelopeAttack);
    addParameter(paramEnvelopeRelease);

    //parameters.valueTreeState.state = ValueTree (Identifier (getName().removeCharacters ("- ")));

    addRefreshParameterFunction([=]{ updateFilters(); });
    setRefreshRate(10);
}

WahWahAudioProcessor::~WahWahAudioProcessor()
{
}

//==============================================================================

void WahWahAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramMode.reset (sampleRate, smoothTime);
    paramMix.reset (sampleRate, smoothTime);
    paramFrequency.reset (sampleRate, smoothTime);
    paramQfactor.reset (sampleRate, smoothTime);
    paramGain.reset (sampleRate, smoothTime);
    paramFilterType.reset (sampleRate, smoothTime);
    paramLFOfrequency.reset (sampleRate, smoothTime);
    paramMixLFOandEnvelope.reset (sampleRate, smoothTime);
    paramEnvelopeAttack.reset (sampleRate, smoothTime);
    paramEnvelopeRelease.reset (sampleRate, smoothTime);*/

    //======================================

    filters.clear();
    for (int i = 0; i < getTotalNumInputChannels(); ++i) {
        Filter* filter;
        filters.add (filter = new Filter());
    }
    updateFilters();

    lfoPhase = 0.0f;
    inverseSampleRate = 1.0f / (float)sampleRate;
    twoPi = 2.0f * M_PI;

    for (int i = 0; i < getTotalNumInputChannels(); ++i)
        envelopes.add (0.0f);
    inverseE = 1.0f / M_E;
}

void WahWahAudioProcessor::releaseResources()
{
}

void WahWahAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    float phase;

    for (int channel = 0; channel < numInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer (channel);
        phase = lfoPhase;

        for (int sample = 0; sample < numSamples; ++sample) {
            float in = channelData[sample];

            float absIn = fabs (in);
            float envelope;
            float attack = calculateAttackOrRelease (*paramEnvelopeAttack * 0.001f);
            float release = calculateAttackOrRelease (* paramEnvelopeRelease * 0.001f);

            if (absIn > envelopes[channel])
                envelope = attack * envelopes[channel] + (1.0f - attack) * absIn;
            else
                envelope = release * envelopes[channel] + (1.0f - release) * absIn;

            envelopes.set (channel, envelope);

            if (*paramMode == modeAutomatic) {
                float centreFrequencyLFO = 0.5f + 0.5f * sinf (twoPi * phase);
                float centreFrequencyEnv = envelopes[channel];
                centreFrequency =
                    centreFrequencyLFO + *paramMixLFOandEnvelope * (centreFrequencyEnv - centreFrequencyLFO);

                centreFrequency *= paramFrequency->getNormalisableRange().end - paramFrequency->getNormalisableRange().start;
                centreFrequency += paramFrequency->getNormalisableRange().start;

                phase += *paramLFOfrequency * inverseSampleRate;
                if (phase >= 1.0f)
                    phase -= 1.0f;

                *paramFrequency = centreFrequency;
                updateFilters();
            }

            float filtered = filters[channel]->processSingleSampleRaw (in);
            float out = in + *paramMix * (filtered - in);
            channelData[sample] = out;
        }
    }

    lfoPhase = phase;

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================

void WahWahAudioProcessor::updateFilters()
{
    double discreteFrequency = 2.0 * M_PI * (double) *paramFrequency / getSampleRate();
    double qFactor = (double) *paramQfactor;
    double gain = pow (10.0, (double) *paramGain * 0.05);
    int type = (int) *paramFilterType;

    for (int i = 0; i < filters.size(); ++i)
        filters[i]->updateCoefficients (discreteFrequency, qFactor, gain, type);
}

float WahWahAudioProcessor::calculateAttackOrRelease (float value)
{
    if (value == 0.0f)
        return 0.0f;
    else
        return pow (inverseE, inverseSampleRate / value);
}

//==============================================================================





