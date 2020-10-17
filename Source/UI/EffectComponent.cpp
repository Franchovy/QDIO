/*
  ==============================================================================

    EffectComponent.cpp
    Created: 15 Oct 2020 11:18:35am
    Author:  maxime

  ==============================================================================
*/

#include "EffectComponent.h"

EffectComponent::EffectComponent() {
    setHoverable(true);
    setSelectable(true);
    setDraggable(true);
    setDragExitable(true);
    setExitDraggable(true);
}


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

void EffectComponent::mouseDown(const MouseEvent &event) {
    if (auto dragPort = dynamic_cast<ConnectionPort*>(event.eventComponent)) {
        dynamic_cast<ConnectionContainer*>(getParentComponent())->startConnectionDrag(
                getParentComponent(),
                dragPort,
                event.getEventRelativeTo(
                        getParentComponent()
                    ).getPosition().toFloat()
                );
    } else {
        SceneComponent::mouseDown(event);
    }
}

void EffectComponent::mouseDrag(const MouseEvent &event) {
    if (dynamic_cast<ConnectionPort*>(event.eventComponent) != nullptr) {
        dynamic_cast<ConnectionContainer*>(getParentComponent())->connectionDrag(
                event.getEventRelativeTo(
                        getParentComponent()).getPosition().toFloat()
                        );
    } else {
        SceneComponent::mouseDrag(event);
    }
}

void EffectComponent::mouseUp(const MouseEvent &event) {
    if (auto dragStartPort = dynamic_cast<ConnectionPort*>(event.eventComponent)) {
        dynamic_cast<ConnectionContainer*>(getParentComponent())->endConnectionDrag(
                event.getEventRelativeTo(
                        getParentComponent()).getPosition().toFloat());
    } else {
        SceneComponent::mouseUp(event);
    }
}

