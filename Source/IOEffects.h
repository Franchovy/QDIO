/*
  ==============================================================================

    IOEffects.h
    Created: 6 Mar 2020 2:44:31pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Settings.h"
#include "BaseEffects.h"

class InputDeviceEffect : public AudioProcessorGraph::AudioGraphIOProcessor
{
public:
    InputDeviceEffect()
        : AudioGraphIOProcessor(AudioGraphIOProcessor::audioInputNode)
        , deviceParam(new AudioParameterChoice("inputdevice", "Device"
                , SettingsComponent::getDevicesList(true), SettingsComponent::getCurrentDeviceIndex(true)))
    {
        addParameter(deviceParam);
    }

    ~InputDeviceEffect() {
        deviceParam->sendValueChangedMessageToListeners(0.0);
    }

    const String getName() const override { return name; }

private:
    const String name = "Input Device";
    AudioParameterChoice* deviceParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputDeviceEffect)
};

class OutputDeviceEffect : public AudioProcessorGraph::AudioGraphIOProcessor//, private BaseEffect
{
public:
    OutputDeviceEffect()
        : AudioGraphIOProcessor(AudioGraphIOProcessor::audioOutputNode)
        , deviceParam(new AudioParameterChoice("outputdevice", "Device"
                , SettingsComponent::getDevicesList(false), SettingsComponent::getCurrentDeviceIndex(false)))
    {
        addParameter(deviceParam);
    }

    void prepareToPlay(double newSampleRate, int estimatedSamplesPerBlock) override {
        AudioGraphIOProcessor::prepareToPlay(newSampleRate, estimatedSamplesPerBlock);
    }

    void releaseResources() override {
        AudioGraphIOProcessor::releaseResources();
    }

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiBuffer) override {
        AudioGraphIOProcessor::processBlock(buffer, midiBuffer);

        auto numIns = getMainBusNumInputChannels();
        auto numOuts = getMainBusNumOutputChannels();
        /*if (numIns > numOuts) {
            for (int i = 0; i < numIns - numOuts; i++)
            {
                buffer.copyFrom(i + numIns, 0, buffer, i, 0, buffer.getNumSamples());
            }
        }*/
    }

    const String getName() const override { return name; }
private:
    const String name = "Output Device";

    AudioParameterChoice* deviceParam;
    String deviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputDeviceEffect)
};

