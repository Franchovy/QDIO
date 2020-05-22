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
    BaseEffect() = default;

    //TODO fix const shit
    const String getName() const override;

    void setLayout(int numInputs, int numOutputs);

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
    BusesLayout layout;
};


class DelayEffect : public BaseEffect, public Timer
{
public:
    DelayEffect();

    void timerCallback() override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

private:
    AudioParameterFloat* delay;
    AudioParameterFloat* fade;
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
    DistortionEffect();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

private:
    AudioParameterFloat* gain;
    AudioParameterFloat* cutoff;
    std::atomic<float> gainVal;

};