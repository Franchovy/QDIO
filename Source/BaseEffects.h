/*
  ==============================================================================

    BaseEffects.h
    Created: 24 Mar 2020 9:46:38am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Parameters.h"

/**
 * Slightly more specialised (but still abstract) AudioProcessor. Avoids having all the same
 * overrides taking up so much space.
 */
class BaseEffect : public AudioProcessor
{
public:
    BaseEffect() : AudioProcessor() {
    }

    ~BaseEffect() override {

    }

    //TODO fix const shit
    const String getName() const override {
        return name;
    }

    void setLayout(int numInputs, int numOutputs) {
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

    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    virtual void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) = 0;

    double getTailLengthSeconds() const override {
        return 0;
    }

    bool acceptsMidi() const override {
        return false;
    }

    bool producesMidi() const override {
        return false;
    }

    AudioProcessorEditor *createEditor() override {
        return nullptr;
    }

    bool hasEditor() const override {
        return false;
    }

    int getNumPrograms() override {
        return 0;
    }

    int getCurrentProgram() override {
        return 0;
    }

    void setCurrentProgram(int index) override {

    }

    const String getProgramName(int index) override {
        return String();
    }

    void changeProgramName(int index, const String &newName) override {

    }

    void getStateInformation(juce::MemoryBlock &destData) override {

    }

    void setStateInformation(const void *data, int sizeInBytes) override {

    }

protected:
    String name;
    ParameterListener parameterListener;
    BusesLayout layout;
};


class DelayEffect : public BaseEffect, public Timer
{
public:
    DelayEffect() : BaseEffect()
                    , delay("length", "Length",
                          NormalisableRange<float>(0.1f, 1.f, 0.001, 0.5f), 0.3f)
                    , fade("fade", "Fade",
                                 NormalisableRange<float>(0, 1.f, 0.001, 0.95f), 0.9f)
    {
        name = "Delay Effect";
        addParameter(&delay);
        delay.addListener(&parameterListener);

        addParameter(&fade);
        fade.addListener(&parameterListener);

        setLayout(1,1);
        startTimer(1000);
    }

    /**
     * Use this to asynchronously update the buffer size
     */
    void timerCallback() override {
        // TODO fix crashing
        if (fadeVal != fade.get()) {
            fadeVal = fade.get();
        }

        newDelayBufferSize = ceil(delay.get() * currentSampleRate );
        if (newDelayBufferSize != delayBufferSize){
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

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
        currentSampleRate = sampleRate;
        numChannels = jmax(getMainBusNumInputChannels(), getMainBusNumOutputChannels());

        // init buffer
        delayBuffer.setSize(numChannels,ceil(delay.range.end * sampleRate)
                , false, true, false);

        std::cout << "max size: " <<  delay.range.end * sampleRate << newLine;

        delayBufferSize = ceil( delay.get() * sampleRate );
        delayBufferPt = 1;

        delayBuffer.setSize(numChannels, delayBufferSize
                , true, false, true);
        delayBuffer.clear();

        fadeVal = fade.get();
    }

    void releaseResources() override {
        std::cout << getName() << ": release resources" << newLine;
    }

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override {
        if (delayBufferSize < buffer.getNumSamples())
            return;


        auto totalNumInputChannels  = getTotalNumInputChannels();
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

private:
    AudioParameterFloat delay;
    AudioParameterFloat fade;
    std::atomic<float> fadeVal;
    //std::atomic<float> delayVal;
    AudioBuffer<float> delayBuffer;

    int minSize;

    int numChannels;
    double currentSampleRate;
    int delayBufferSize;
    int newDelayBufferSize;
    int delayBufferPt;


};

class DistortionEffect : public BaseEffect {
public:
    DistortionEffect() : BaseEffect(),
                         gain("gain", "Gain",
                              NormalisableRange<float>(0.0f, 2.f, 0.001, 1.0f), 1.0f),
                         cutoff("cutoff", "Cutoff",
                              NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.8f)
              {
        name = "Distortion Effect";
        addParameter(&gain);
        gain.addListener(&parameterListener);
        addParameter(&cutoff);
        cutoff.addListener(&parameterListener);
        setLayout(1,1);
    }


    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
        gainVal = gain;
    }

    void releaseResources() override {

    }

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override
    {
        for (int c = 0; c < buffer.getNumChannels(); c++) {
            for (int s = 0; s < buffer.getNumSamples(); s++){

                float sample = jlimit(-cutoff.get(), cutoff.get(), buffer.getSample(c, s) * gain);
                sample *= gain;
                buffer.setSample(c, s, sample);
            }
        }
    }

private:
    AudioParameterFloat gain;
    AudioParameterFloat cutoff;
    std::atomic<float> gainVal;

};