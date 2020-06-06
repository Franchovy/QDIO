/*
  ==============================================================================

    BaseEffects.cpp
    Created: 24 Mar 2020 9:46:38am
    Author:  maxime

  ==============================================================================
*/

#include "BaseEffects.h"

const String BaseEffect::getName() const {
    return name;
}

void BaseEffect::setLayout(int numInputs, int numOutputs) {
    auto defaultInChannel = AudioChannelSet();
    defaultInChannel.addChannel(AudioChannelSet::ChannelType::left);
    defaultInChannel.addChannel(AudioChannelSet::ChannelType::right);
    auto defaultOutChannel = AudioChannelSet();
    defaultOutChannel.addChannel(AudioChannelSet::ChannelType::left);
    defaultOutChannel.addChannel(AudioChannelSet::ChannelType::right);

    for (int i = 0; i < numInputs; i++) {
        layout.inputBuses.add(defaultInChannel);
    }
    for (int i = 0; i < numOutputs; i++) {
        layout.outputBuses.add(defaultOutChannel);
    }
    setBusesLayout(layout);
}

void BaseEffect::makeLog(AudioParameterFloat *parameter) {
    auto range = parameter->getNormalisableRange();
    parameter->range.setSkewForCentre (sqrt (range.start * range.end));
}

DelayEffect::DelayEffect() : BaseEffect()
        , delay(new AudioParameterFloat("length", "Length",
                                        NormalisableRange<float>(0.1f, 1.f, 0.001, 0.5f), 0.3f))
        , fade(new AudioParameterFloat("fade", "Fade",
                                       NormalisableRange<float>(0, 1.f, 0.001, 0.95f), 0.9f))
{
    name = "Delay Effect";

    addParameter(delay);
    addParameter(fade);

    setLayout(1,1);
    startTimer(1000);
}


/**
 * Use this to asynchronously update the buffer size
 */
void DelayEffect::timerCallback() {
    if (fadeVal != fade->get()) {
        fadeVal = fade->get();
    }


    newDelayBufferSize = ceil(delay->get() * currentSampleRate );
    if (newDelayBufferSize != delayBufferSize){
        if (numChannels < 0 || newDelayBufferSize < 0) {
            startTimer(1000);
            return;
        }


        std::cout << "Updating buffer size to: " << newDelayBufferSize << newLine;

        delayBuffer.setSize(numChannels, newDelayBufferSize
                , true, true, true);
        if (delayBufferPt > newDelayBufferSize){
            delayBufferPt = 1;
        }
        /*delayBuffer.clear();*/

        delayBufferSize = newDelayBufferSize;
    }
    startTimer(500);

}

void DelayEffect::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    currentSampleRate = sampleRate;
    numChannels = jmax(getMainBusNumInputChannels(), getMainBusNumOutputChannels());

    // init buffer
    delayBuffer.setSize(numChannels,ceil(delay->range.end * sampleRate)
            , false, true, false);

    std::cout << "max size: " <<  delay->range.end * sampleRate << newLine;

    delayBufferSize = ceil( delay->get() * sampleRate );
    delayBufferPt = 1;

    delayBuffer.setSize(numChannels, delayBufferSize
            , false, false, true);
    delayBuffer.clear();

    fadeVal = fade->get();
}

void DelayEffect::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) {
    if (! bypass->get()) {
        if (delayBufferSize < buffer.getNumSamples())
            return;


        auto totalNumInputChannels = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();
        auto numSamples = buffer.getNumSamples();

        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            auto *channelData = buffer.getWritePointer(channel);
            bool resetState = false;
            if (channel == totalNumInputChannels - 1) {
                resetState = true;
            }

            if (delayBufferPt + numSamples > delayBufferSize) {
                auto firstHalfSize = delayBufferSize - delayBufferPt;
                auto secondHalfSize = numSamples - firstHalfSize;
                // Split add to output
                buffer.addFrom(channel, 0, delayBuffer, channel, delayBufferPt, firstHalfSize);
                buffer.addFrom(channel, firstHalfSize, delayBuffer, channel, 0, secondHalfSize);
                // Copy over to delayBuffer
                delayBuffer.copyFrom(channel, delayBufferPt, buffer, channel, 0, firstHalfSize);
                delayBuffer.copyFrom(channel, 0, buffer, channel, firstHalfSize, secondHalfSize);
                // Apply gain to echo buffer
                delayBuffer.applyGain(delayBufferPt, firstHalfSize, fadeVal);
                delayBuffer.applyGain(channel, 0, secondHalfSize, fadeVal);
                // Set new delayBufferPt if this is the last channel
                if (resetState)
                    delayBufferPt = numSamples - firstHalfSize;
            } else {
                // Add to output
                buffer.addFrom(channel, 0, delayBuffer, channel, delayBufferPt, numSamples);
                // Copy to echo buffer
                delayBuffer.copyFrom(channel, delayBufferPt, buffer, channel, 0, numSamples);
                // Apply gain to echo buffer
                delayBuffer.applyGain(channel, delayBufferPt, numSamples, fadeVal);
                // Set new delayBufferPt if this is the last channel
                if (resetState)
                    delayBufferPt += numSamples;
            }
        }
    }
}

void DelayEffect::releaseResources() {
    delayBuffer.clear();
}

DistortionEffect::DistortionEffect() : BaseEffect()
                                , gain(new AudioParameterFloat("gain", "Gain"
                                        , NormalisableRange<float>(0.0f, 2.f, 0.001, 1.0f), 1.0f))
                                , cutoff(new AudioParameterFloat("cutoff", "Cutoff"
                                        ,NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.8f))
                                , postGain(new AudioParameterFloat("postGain", "Post Gain"
                                        ,NormalisableRange<float>(0.0f, 2.0f, 0.001f, 1.0f), 1.3f))
{
    name = "Distortion Effect";
    addParameter(gain);
    addParameter(cutoff);
    addParameter(postGain);

    setLayout(1,1);
}

void DistortionEffect::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    gainVal = *gain;
}

void DistortionEffect::releaseResources() {

}

void DistortionEffect::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) {
    if (! bypass->get()) {
        for (int c = 0; c < buffer.getNumChannels(); c++) {
            for (int s = 0; s < buffer.getNumSamples(); s++) {

                float sample = jlimit(-cutoff->get(), cutoff->get(), buffer.getSample(c, s) * *gain);
                sample *= *postGain;
                buffer.setSample(c, s, sample);
            }
        }
    }
}
