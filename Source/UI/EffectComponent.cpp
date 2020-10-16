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
        inPort->setSize(40,40);
        inPort->geometry.boundarySize = 10;
        inPort->setCentrePosition(geometry.boundarySize, inY);
        inPort->setHoverable(true);
        addAndMakeVisible(inPort);
        inY += 50;
    }
    int outY = 50;
    for (auto outPort : ports.outPorts) {
        outPort->setSize(40,40);
        outPort->geometry.boundarySize = 10;
        outPort->setCentrePosition(getWidth() - geometry.boundarySize, outY);
        outPort->setHoverable(true);
        addAndMakeVisible(outPort);
        outY += 50;
    }
}
