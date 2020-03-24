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
    DelayEffect() : BaseEffect()
    {
        name = "Delay Effect";
    }

private:

};