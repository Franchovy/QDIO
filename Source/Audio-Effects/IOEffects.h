/*
  ==============================================================================

    IOEffects.h
    Created: 6 Mar 2020 2:44:31pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../Settings.h"
#include "../BaseEffects.h"

class IOEffect : public AudioProcessorGraph::AudioGraphIOProcessor, public AudioProcessorParameter::Listener
{
public:
    IOEffect(bool isInput)
        : AudioGraphIOProcessor(isInput ? AudioGraphIOProcessor::audioInputNode : AudioGraphIOProcessor::audioOutputNode)
        , deviceParam(new AudioParameterChoice(isInput ? "inputdevice" : "outputDevice", "Device"
            , SettingsComponent::getDevicesList(isInput), SettingsComponent::getCurrentDeviceIndex(isInput)))
        , forceStereo(new AudioParameterBool("Force Stereo", "forcestereo", isInput))
    {
        this->isInput = isInput;
        addParameter(deviceParam);
        addParameter(forceStereo);

        if (isInput) {
            setChannelLayoutOfBus(false, 0, AudioChannelSet::stereo());
        }


        deviceParam->addListener(this);
    }

    void prepareToPlay(double newSampleRate, int estimatedSamplesPerBlock) override {

        if (isInput && getMainBusNumOutputChannels() == 1) {
            needsStereoForcing = true;

            getBusesLayout().getMainOutputChannelSet().addChannel(AudioChannelSet::right);

            stereoBuffer = AudioBuffer<float>(2, estimatedSamplesPerBlock);
        }

        AudioGraphIOProcessor::prepareToPlay(newSampleRate, estimatedSamplesPerBlock);
    }

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiBuffer) override {
        if (! isInput) {
            if (*forceStereo && buffer.getNumChannels() > 1) {
                buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
            }/* else if (! *forceStereo) {
                for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); i++) {
                    buffer.clear(i, buffer.getNumSamples());
                }
            }*/
        }

        AudioGraphIOProcessor::processBlock(buffer, midiBuffer);

        if (isInput) {
            if (*forceStereo && needsStereoForcing) {
                stereoBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
                stereoBuffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

                buffer = stereoBuffer;
            }
        }
    }

    void parameterValueChanged(int parameterIndex, float newValue) override {
        SettingsComponent::setCurrentDevice(isInput, newValue);
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        //todo update list
    }

private:
    bool isInput;

    AudioBuffer<float> stereoBuffer;

    AudioParameterChoice* deviceParam;
    AudioParameterBool* forceStereo;

    bool needsStereoForcing = false;
};

/*class InputDeviceEffect : public AudioProcessorGraph::AudioGraphIOProcessor
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
        *//*if (numIns > numOuts) {
            for (int i = 0; i < numIns - numOuts; i++)
            {
                buffer.copyFrom(i + numIns, 0, buffer, i, 0, buffer.getNumSamples());
            }
        }*//*
    }

    const String getName() const override { return name; }
private:
    const String name = "Output Device";

    AudioParameterChoice* deviceParam;
    String deviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputDeviceEffect)
};*/

