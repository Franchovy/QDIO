/*
  ==============================================================================

    IOEffects.h
    Created: 6 Mar 2020 2:44:31pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class InputDeviceEffect : public AudioProcessorGraph::AudioGraphIOProcessor
{
public:
    InputDeviceEffect() : AudioGraphIOProcessor(AudioGraphIOProcessor::audioInputNode){

    }

    const String getName() const override { return name; }

private:
    const String name = "Audio Device Input";
};

class OutputDeviceEffect : public AudioProcessorGraph::AudioGraphIOProcessor
{
public:
    OutputDeviceEffect() : AudioGraphIOProcessor(AudioGraphIOProcessor::audioOutputNode){

    }

    const String getName() const override { return name; }
private:
    const String name = "Audio Device Output";
};

