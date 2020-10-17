/*
  ==============================================================================

    EffectConnectable.cpp
    Created: 15 Oct 2020 11:17:47am
    Author:  maxime

  ==============================================================================
*/

#include "EffectConnectable.h"

EffectConnectable::EffectConnectable() {

}

void EffectConnectable::setNumPorts(int numIn, int numOut) {
    if (numIn > ports.inPorts.size()) {
        // Add input ports
        for (int i = ports.inPorts.size(); i < numIn; i++) {
            ports.inPorts.add(createPort(true));
        }
    } else if (numIn < ports.inPorts.size()){
        // Remove input ports
        for (int i = ports.inPorts.size(); i > numIn; i++) {
            ports.inPorts.removeObject(ports.inPorts[i]);
        }
    }
    if (numOut > ports.outPorts.size()) {
        // Add output ports
        for (int i = ports.outPorts.size(); i < numOut; i++) {
            ports.outPorts.add(createPort(false));
        }
    } else if (numOut < ports.outPorts.size()){
        // Remove output ports
        for (int i = ports.outPorts.size(); i > numOut; i++) {
            ports.outPorts.removeObject(ports.outPorts[i]);
        }
    }
}

std::unique_ptr<ConnectionPort> EffectConnectable::createPort(bool isInput) {
    return std::make_unique<ConnectionPort>();
}
