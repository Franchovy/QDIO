/*
  ==============================================================================

    IOEffects.h
    Created: 6 Mar 2020 2:44:31pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "BaseEffects.h"

class InputDeviceEffect : public AudioProcessorGraph::AudioGraphIOProcessor
{
public:
    InputDeviceEffect(StringArray choices, int defaultIndex)
        : AudioGraphIOProcessor(AudioGraphIOProcessor::audioInputNode)
        , deviceParam(new AudioParameterChoice("inputdevice", "Device", choices, defaultIndex))
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

};

class OutputDeviceEffect : public AudioProcessorGraph::AudioGraphIOProcessor//, private BaseEffect
{
public:
    OutputDeviceEffect(StringArray choices, int defaultIndex)
        : AudioGraphIOProcessor(AudioGraphIOProcessor::audioOutputNode)
        , deviceParam(new AudioParameterChoice("outputdevice", "Device", choices, defaultIndex))
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
};

