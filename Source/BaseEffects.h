/*
  ==============================================================================

    BaseEffects.h
    Created: 24 Mar 2020 9:46:38am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ParameterListener : public AudioProcessorParameter::Listener
{
public:
    ParameterListener() : AudioProcessorParameter::Listener() {

    }

    void parameterValueChanged(int parameterIndex, float newValue) override {
        *parameters[parameterIndex] = newValue;
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {

    }

    Array<std::atomic<float>*> parameters;
};

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
    ParameterListener parameterListener;
};


class DelayEffect : public BaseEffect, public Timer
{
public:
    DelayEffect() : BaseEffect(),
                    delay("delay", "Delay",
                          NormalisableRange<float>(0, 2.f, 0.001, 0.5f), 0.1f)
    {
        name = "Delay Effect";
        addParameter(&delay);
        delay.addListener(&parameterListener);
        //parameterListener.parameters.add(&delayVal);
        startTimer(1000);
    }

    /**
     * Use this to asynchronously update the buffer size
     */
    void timerCallback() override {
        // TODO fix crashing

        newDelayBufferSize = ceil(delay.get() * currentSampleRate );
        if (newDelayBufferSize != delayBufferSize){
            std::cout << "Updating buffer size to: " << newDelayBufferSize << newLine;


            delayBuffer.setSize(jmin(getMainBusNumInputChannels(), getMainBusNumOutputChannels()),
                                newDelayBufferSize, true, true, true);
            if (delayBufferPt > newDelayBufferSize){
                delayBufferPt = 1;
            }
            delayBuffer.clear();

            delayBufferSize = newDelayBufferSize;
        }
        startTimer(500);
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
        currentSampleRate = sampleRate;

        delayBufferSize = ceil( delay.get() * sampleRate );
        delayBufferPt = 1;

        delayBuffer.setSize(jmax(getMainBusNumInputChannels(), getMainBusNumOutputChannels()),
                delayBufferSize, true, false, true);
        delayBuffer.clear();
    }

    void releaseResources() override {
        std::cout << getName() << ": release resources" << newLine;
    }

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override {
        if (delayBufferSize == 0)
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
                delayBuffer.applyGain(delayBufferPt, firstHalfSize, 0.9);
                delayBuffer.applyGain(channel, 0, secondHalfSize, 0.9);
                // Set new delayBufferPt if this is the last channel
                if (resetState)
                    delayBufferPt = numSamples - firstHalfSize;
            } else {
                // Add to output
                buffer.addFrom(channel, 0, delayBuffer, channel, delayBufferPt, numSamples);
                // Copy to echo buffer
                delayBuffer.copyFrom(channel, delayBufferPt, buffer, channel, 0, numSamples);
                // Apply gain to echo buffer
                delayBuffer.applyGain(channel, delayBufferPt, numSamples, 0.9);
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

    double currentSampleRate;
    int delayBufferSize;
    int newDelayBufferSize;
    int delayBufferPt;

    std::atomic<float> delayVal;
};

class DistortionEffect : public BaseEffect {
public:
    DistortionEffect() : BaseEffect(),
                         gain("gain", "Gain",
                              NormalisableRange<float>(0, 2.f, 0.001, 0.5f), 0.1f) {
        name = "Gain Effect";
        addParameter(&gain);
        gain.addListener(&parameterListener);
    }


    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
        gainVal = gain;
    }

    void releaseResources() override {

    }

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override
    {
        buffer.applyGain(gain);
    }

private:
    AudioParameterFloat gain;
    std::atomic<float> gainVal;

};