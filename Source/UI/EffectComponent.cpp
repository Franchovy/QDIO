/*
  ==============================================================================

    EffectComponent.cpp
    Created: 15 Oct 2020 11:18:35am
    Author:  maxime

  ==============================================================================
*/

#include "EffectComponent.h"

void EffectComponent::setNumPorts(int numIn, int numOut) {
    EffectConnectable::setNumPorts(numIn, numOut);

    int inY = 50;
    for (auto inPort : ports.inPorts) {
        inPort->setBounds(20, inY, 60,60);
        inPort->setHoverable(true);
        addAndMakeVisible(inPort);
        inY += 50;
    }
    int outY = 50;
    for (auto outPort : ports.outPorts) {
        outPort->setBounds(getWidth()-20, outY, 60,60);
        outPort->setHoverable(true);
        addAndMakeVisible(outPort);
        outY += 50;
    }
}
