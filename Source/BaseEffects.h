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
class BaseEffect : public AudioProcessor, public Timer, public AsyncUpdater, public AudioProcessorParameter::Listener
{
public:
    BaseEffect(String processorName)
        : AudioProcessor()
        , bypass(new AudioParameterBool("bypass", "Bypass", false))
    {
        name = processorName;
        addParameter(bypass);
    }

    //=========================================================================
    void addOutputParameter(AudioProcessorParameter* parameter);
    Array<AudioProcessorParameter*> outputParameters;

    //=========================================================================

    //TODO fix const shit
    const String getName() const override;

    //=========================================================================

    void setLayout(int numInputs, int numOutputs);
    void makeLog(AudioParameterFloat* parameter);

    //=========================================================================
    void timerCallback() override;

    //=========================================================================
    virtual void handleAsyncUpdate() override;

    //=========================================================================

    void addParameterListener(AudioProcessorParameter* parameter);
    virtual void parameterValueChanged(int parameterIndex, float newValue) override;
    virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    //=========================================================================

    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override = 0;
    virtual void releaseResources() override = 0;
    virtual void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override = 0;

    void processBlockBypassed(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override;

    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    AudioProcessorEditor *createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override { }

    const String getProgramName(int index) override { return String(); }
    void changeProgramName(int index, const String &newName) override { }

    void getStateInformation(juce::MemoryBlock &destData) override { }
    void setStateInformation(const void *data, int sizeInBytes) override { }

    AudioProcessorParameter *getBypassParameter() const override;

protected:
    AudioParameterBool* bypass;

    String name;
    BusesLayout layout;

    void addRefreshParameterFunction(std::function<void()> function);
    void setRefreshRate(int refreshRateInHz);
private:
    Array<std::function<void()>> refreshParameterFunctions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseEffect)
};
