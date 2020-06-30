/*
  ==============================================================================

    BaseEffects.cpp
    Created: 24 Mar 2020 9:46:38am
    Author:  maxime

  ==============================================================================
*/

#include "BaseEffects.h"

const String BaseEffect::getName() const {
    return name;
}

void BaseEffect::setLayout(int numInputs, int numOutputs) {
    auto emptyChannel = AudioChannelSet();
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
    if (numInputs == 0) {
        layout.inputBuses.add(emptyChannel);
    }
    if (numOutputs == 0) {
        layout.outputBuses.add(emptyChannel);
    }

    setBusesLayout(layout);
}

void BaseEffect::makeLog(AudioParameterFloat *parameter) {
    auto range = parameter->getNormalisableRange();
    parameter->range.setSkewForCentre (sqrt (range.start * range.end));
}

void BaseEffect::addRefreshParameterFunction(std::function<void()> function) {
    refreshParameterFunctions.add(function);
}

void BaseEffect::setRefreshRate(int refreshRateInHz) {
    startTimerHz(refreshRateInHz);
}

void BaseEffect::timerCallback() {
    for (auto && refreshParameterFunction : refreshParameterFunctions) {
        refreshParameterFunction();
    }
}

void BaseEffect::addOutputParameter(AudioProcessorParameter *parameter) {
    outputParameters.add(parameter);
    addParameter(parameter);
}
