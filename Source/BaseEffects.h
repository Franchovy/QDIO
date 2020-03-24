/*
  ==============================================================================

    BaseEffects.h
    Created: 24 Mar 2020 9:46:38am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
 * Slightly more specialised (but still abstract) AudioProcessor. Avoids having all the same
 * overrides taking up so much space.
 */
class BaseEffect : public AudioProcessor
{
public:
    BaseEffect() : AudioProcessor() {

    }

    //TODO fix const shit
    const String getName() const override {
        return name;
    }

    void setLayout(int numInputs, int numOutputs) {
        BusesLayout layout;
        for (int i = 0; i < numInputs; i++)
            layout.inputBuses.add(layout.getMainInputChannelSet());
        for (int i = 0; i < numOutputs; i++)
            layout.outputBuses.add(layout.getMainOutputChannelSet());
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
};


class DelayEffect : public BaseEffect
{
public:
    DelayEffect() : BaseEffect(),
                    delay("delay", "Delay",
                          NormalisableRange<float>(0, 2.f, 0.05, 0.5f), 0.1f)
    {
        name = "Delay Effect";
        addParameter(&delay);
    }


    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
        delayBufferSize = delay.get() * sampleRate;
        delayBufferPt = 0;

        delayBuffer.setSize(jmin(getMainBusNumInputChannels(), getMainBusNumOutputChannels()),
                delayBufferSize, true, false, true);
        delayBuffer.clear();
    }

    void releaseResources() override {

    }

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override {
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();
        auto numSamples = buffer.getNumSamples();

        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            auto *channelData = buffer.getWritePointer(channel);
            bool resetState = false;
            if (channel == totalNumInputChannels - 1) {
                resetState = true;
            }
            //TODO change this into delay instead of echo.
            if (delayBufferPt + numSamples > delayBufferSize) {
                auto firstHalfSize = delayBufferSize - delayBufferPt;
                auto secondHalfSize = numSamples - firstHalfSize;
                // Split add to output
                buffer.addFrom(channel, 0, delayBuffer, channel, delayBufferPt, firstHalfSize);
                buffer.addFrom(channel, firstHalfSize, delayBuffer, channel, 0, secondHalfSize);
                // Copy over to delayBuffer
                delayBuffer.copyFrom(channel, delayBufferPt, buffer, channel, 0, firstHalfSize);
                delayBuffer.copyFrom(channel, 0, buffer, channel, firstHalfSize, secondHalfSize);
                // Set new delayBufferPt if this is the last channel
                if (resetState)
                    delayBufferPt = numSamples - firstHalfSize;
            } else {
                // Add to output
                buffer.addFrom(channel, 0, delayBuffer, channel, delayBufferPt, numSamples);
                // Copy to echo buffer
                delayBuffer.copyFrom(channel, delayBufferPt, buffer, channel, 0, numSamples);
                // Set new delayBufferPt if this is the last channel
                if (resetState)
                    delayBufferPt += numSamples;
            }
        }
    }

private:
    AudioParameterFloat delay;
    //std::atomic<float> delayVal;
    AudioBuffer<float> delayBuffer;

    int delayBufferSize;
    int delayBufferPt;
};