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
#include "ConnectionContainer.h"

struct EffectPorts {
    OwnedArray<ConnectionPort> inPorts;
    OwnedArray<ConnectionPort> outPorts;
};

class EffectConnectable
{
public:
    EffectConnectable();

    virtual void setNumPorts(int numIn, int numOut);

    void addConnectionToThis(Connection* newConnection);
    void moveConnectedComponent(SceneComponent* component, Point<int> delta);

    Array<ConnectionPort*> getPorts();

protected:
    EffectPorts ports;

private:
    std::unique_ptr<ConnectionPort> createPort(bool isInput);
    ReferenceCountedArray<Connection> connections;
};