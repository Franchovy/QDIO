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

#include "Delay.h"

//==============================================================================

DelayAudioProcessor::DelayAudioProcessor():
    BaseEffect("Delay")
    , paramDelayTime (new AudioParameterFloat("Delay time", "length", NormalisableRange<float>(0.0f, 5.0f, 0.001f), 0.1f, "s"))
    , paramFeedback (new AudioParameterFloat("Feedback", "feedback", NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f, ""))
    , paramMix (new AudioParameterFloat("Mix", "mix", NormalisableRange<float>(0.0f, 1.0f, 0.01), 1.0f))
{
    setLayout(1,1);
    addParameter(paramDelayTime);
    addParameter(paramFeedback);
    addParameter(paramMix);
}

DelayAudioProcessor::~DelayAudioProcessor()
{
}

//==============================================================================

void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramDelayTime.reset (sampleRate, smoothTime);
    paramFeedback.reset (sampleRate, smoothTime);
    paramMix.reset (sampleRate, smoothTime);
*/
    //======================================

    float maxDelayTime = paramDelayTime->getNormalisableRange().end;
    delayBufferSamples = (int)(maxDelayTime * (float)sampleRate) + 1;
    if (delayBufferSamples < 1)
        delayBufferSamples = 1;

    delayBufferChannels = getTotalNumInputChannels();
    delayBuffer.setSize (delayBufferChannels, delayBufferSamples);
    delayBuffer.clear();

    delayWritePosition = 0;
}

void DelayAudioProcessor::releaseResources()
{
}

void DelayAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    if (! *bypass) {
        ScopedNoDenormals noDenormals;

        const int numInputChannels = getTotalNumInputChannels();
        const int numOutputChannels = getTotalNumOutputChannels();
        const int numSamples = buffer.getNumSamples();

        //======================================

        float currentDelayTime = *paramDelayTime * (float) getSampleRate();
        float currentFeedback = *paramFeedback;
        float currentMix = *paramMix;

        int localWritePosition;

        for (int channel = 0; channel < numInputChannels; ++channel) {
            float *channelData = buffer.getWritePointer(channel);
            float *delayData = delayBuffer.getWritePointer(channel);
            localWritePosition = delayWritePosition;

            for (int sample = 0; sample < numSamples; ++sample) {
                const float in = channelData[sample];
                float out = 0.0f;

                float readPosition =
                        fmodf((float) localWritePosition - currentDelayTime + (float) delayBufferSamples,
                              delayBufferSamples);
                int localReadPosition = floorf(readPosition);

                if (localReadPosition != localWritePosition) {
                    float fraction = readPosition - (float) localReadPosition;
                    float delayed1 = delayData[(localReadPosition + 0)];
                    float delayed2 = delayData[(localReadPosition + 1) % delayBufferSamples];
                    out = delayed1 + fraction * (delayed2 - delayed1);

                    channelData[sample] = in + currentMix * (out - in);
                    delayData[localWritePosition] = in + out * currentFeedback;
                }

                if (++localWritePosition >= delayBufferSamples)
                    localWritePosition -= delayBufferSamples;
            }
        }

        delayWritePosition = localWritePosition;

        //======================================

        for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
            buffer.clear(channel, 0, numSamples);
    }
}

//==============================================================================






#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
