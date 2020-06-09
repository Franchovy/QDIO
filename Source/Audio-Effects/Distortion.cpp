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

#include "Distortion.h"

//==============================================================================

DistortionAudioProcessor::DistortionAudioProcessor():
    BaseEffect()
    , paramDistortionType (new AudioParameterChoice("Distortion type", "disttype",
            distortionTypeItemsUI, distortionTypeFullWaveRectifier))
    , paramInputGain (new AudioParameterFloat("Input gain", "inputgain",
            NormalisableRange<float>(-24.0f,  24.0f, 0.01f),  12.0f, "dB"))
    , paramOutputGain (new AudioParameterFloat("Output gain", "outputgain",
            NormalisableRange<float>(-24.0f, 24.0f,  0.01f), -24.0f,  "dB"))
    , paramTone (new AudioParameterFloat("Tone", "tone", NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 12.0f, "dB"))
                 //[this](float value){ paramTone.setCurrentAndTargetValue (value); updateFilters(); return value; })
{
    setLayout(1,1);
    addParameter(paramDistortionType);
    addParameter(paramInputGain);
    addParameter(paramOutputGain);
    addParameter(paramTone);

    startTimer(500);
}

DistortionAudioProcessor::~DistortionAudioProcessor()
{
}

//==============================================================================

void DistortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramDistortionType.reset (sampleRate, smoothTime);
    paramInputGain.reset (sampleRate, smoothTime);
    paramOutputGain.reset (sampleRate, smoothTime);
    paramTone.reset (sampleRate, smoothTime);
*/
    //======================================

    filters.clear();
    for (int i = 0; i < getTotalNumInputChannels(); ++i) {
        Filter* filter;
        filters.add (filter = new Filter());
    }
    updateFilters();
}

void DistortionAudioProcessor::releaseResources()
{
}

void DistortionAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    for (int channel = 0; channel < numInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer (channel);

        float out;
        for (int sample = 0; sample < numSamples; ++sample) {
            const float in = channelData[sample] * powf(10.0f, *paramInputGain * 0.05f);

            switch ((int) paramDistortionType->getIndex()) {
                case distortionTypeHardClipping: {
                    float threshold = 0.5f;
                    if (in > threshold)
                        out = threshold;
                    else if (in < -threshold)
                        out = -threshold;
                    else
                        out = in;
                    break;
                }
                case distortionTypeSoftClipping: {
                    float threshold1 = 1.0f / 3.0f;
                    float threshold2 = 2.0f / 3.0f;
                    if (in > threshold2)
                        out = 1.0f;
                    else if (in > threshold1)
                        out = 1.0f - powf (2.0f - 3.0f * in, 2.0f) / 3.0f;
                    else if (in < -threshold2)
                        out = -1.0f;
                    else if (in < -threshold1)
                        out = -1.0f + powf (2.0f + 3.0f * in, 2.0f) / 3.0f;
                    else
                        out = 2.0f * in;
                    out *= 0.5f;
                    break;
                }
                case distortionTypeExponential: {
                    if (in > 0.0f)
                        out = 1.0f - expf (-in);
                    else
                        out = -1.0f + expf (in);
                    break;
                }
                case distortionTypeFullWaveRectifier: {
                    out = fabsf (in);
                    break;
                }
                case distortionTypeHalfWaveRectifier: {
                    if (in > 0.0f)
                        out = in;
                    else
                        out = 0.0f;
                    break;
                }
            }

            float filtered = filters[channel]->processSingleSampleRaw (out);
            channelData[sample] = filtered * powf(10.0f, *paramOutputGain * 0.05f);
        }
    }

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================

void DistortionAudioProcessor::updateFilters()
{
    double discreteFrequency = M_PI * 0.01;
    double gain = pow (10.0, (double) *paramTone * 0.05);

    for (int i = 0; i < filters.size(); ++i)
        filters[i]->updateCoefficients (discreteFrequency, gain);
}

//==============================================================================


#ifndef JucePlugin_PreferredChannelConfigurations
bool DistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DistortionAudioProcessor::timerCallback() {
    updateFilters();
    startTimer(500);
}

#endif
