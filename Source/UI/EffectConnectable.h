/*
  ==============================================================================

    EffectConnectable.h
    Created: 15 Oct 2020 11:17:47am
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "ConnectionPort.h"

struct EffectPorts {
    OwnedArray<ConnectionPort> inPorts;
    OwnedArray<ConnectionPort> outPorts;
};

class EffectConnectable
{
public:
    EffectConnectable();

    void setNumPorts(int numIn, int numOut);
    void autoPositionPorts();

protected:
    EffectPorts ports;

private:
    std::unique_ptr<ConnectionPort> createPort(bool isInput);

};