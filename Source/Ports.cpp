/*
  ==============================================================================

    Ports.cpp
    Created: 25 Apr 2020 11:41:27am
    Author:  maxime

  ==============================================================================
*/

#include "Ports.h"

#include "Effect.h"

AudioPort::AudioPort(bool isInput) : ConnectionPort()
        , internalPort(new InternalConnectionPort(this, !isInput))
{
    linkedPort = internalPort.get();
    isInternal = false;

    hoverBox = Rectangle<int>(0,0,60,60);
    outline = Rectangle<int>(20, 20, 20, 20);
    centrePoint = Point<int>(30,30);
    setBounds(0,0,60, 60);

    this->isInput = isInput;
}

bool AudioPort::canConnect(const ConnectionPort* other) const {
    // Cannot connect to two of the same type.
    if (this->isInput == other->isInput) {
        return false;
    }

    // Cannot connect to same parent.
    if (this->getParentComponent() == other->getParentComponent()) {
        return false;
    }

    // Connect to AudioPort of mutual parent
    return (dynamic_cast<const AudioPort *>(other)
            && other->getParentComponent()->getParentComponent() == this->getParentComponent()->getParentComponent())
           // Connect to ICP of containing parent effect
           || (dynamic_cast<const InternalConnectionPort *>(other)
               && other->getParentComponent() == this->getParentComponent()->getParentComponent());
}

Component *AudioPort::getDragLineParent() {
    return getParentComponent()->getParentComponent();
}


void ConnectionPort::paint(Graphics &g) {
    g.setColour(Colours::whitesmoke);
    g.fillRect(outline);

    g.setColour(findColour(ColourIDs::portColour));
    g.drawRect(outline,2);

    // Hover rectangle
    g.setColour(Colours::blue);
    Path drawPath;
    drawPath.addRoundedRectangle(hoverBox, 10, 10);
    PathStrokeType strokeType(3);

    if (hoverMode) {
        float thiccness[] = {2, 3};
        strokeType.createDashedStroke(drawPath, drawPath, thiccness, 2);
        g.strokePath(drawPath, strokeType);
    }
}

void ConnectionPort::mouseDown(const MouseEvent &event) {
    getParentComponent()->mouseDown(event);
}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    getParentComponent()->mouseDrag(event);
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    getParentComponent()->mouseUp(event);
}

ConnectionPort::ConnectionPort() {
    setColour(portColour, Colours::black);
}

ConnectionPort *ConnectionPort::getLinkedPort() const {
    return linkedPort;
}


ConnectionPort::Ptr ConnectionPort::getOtherPort() {
    return otherPort;
}

void ConnectionPort::setOtherPort(ConnectionPort *newPort) {
    otherPort = newPort;
}

void ConnectionPort::setLinkedPort(ConnectionPort *port) {
    linkedPort = port;
}

Effect *ConnectionPort::getParentEffect() {
    return dynamic_cast<Effect*>(getParentComponent());
}

bool InternalConnectionPort::canConnect(const ConnectionPort* other) const {
    if (dynamic_cast<const ParameterPort*>(other)) {
        return false;
    }
    // Return false if the port is AP and belongs to the same parent
    return (other->isInput ^ isInput)
        && ! (dynamic_cast<const AudioPort *>(other)
             && this->getParentComponent() == other->getParentComponent());
}

InternalConnectionPort::InternalConnectionPort(AudioPort *parent, bool isInput) : ConnectionPort() {
    audioPort = parent;
    linkedPort = audioPort;

    this->isInput = isInput;
    isInternal = true;

    hoverBox = Rectangle<int>(0,0,30,30);
    outline = Rectangle<int>(10,10,10,10);
    centrePoint = Point<int>(15,15);
    //setBounds(parent->getX(), parent->getY(), 30, 30);
    setBounds(0, 0, 30, 30);
}

Component *InternalConnectionPort::getDragLineParent() {
    return getParentComponent();
}

bool ParameterPort::canConnect(const ConnectionPort* other) const {
    if (auto p = dynamic_cast<const ParameterPort*>(other)) {
        if (p->getParentComponent() == getLinkedPort()->getParentComponent()) {
            return false;
        }

        return isInput ^ p->isInput;
    }
    return false;
}

ParameterPort::ParameterPort(bool isInternal, SelectHoverObject* parameterParent)
    : ConnectionPort()
    , parameterParent(parameterParent)
{
    hoverBox = Rectangle<int>(0,0,36,36);
    outline = Rectangle<int>(12,12,12,12);
    centrePoint = Point<int>(18,18);

    isInput = isInternal;
    this->isInternal = isInternal;

    setBounds(0, 0, 36, 36);
}

Component *ParameterPort::getDragLineParent() {
    return getParentComponent()->getParentComponent();
}

Effect* ParameterPort::getParentEffect() {
    if (isInternal) {
        return dynamic_cast<Effect*>(getParentComponent());
    } else {
        return dynamic_cast<Effect*>(getParentComponent()->getParentComponent());
    }
}

void ParameterPort::mouseEnter(const MouseEvent &event) {
    SelectHoverObject::mouseEnter(event);

    if (isInternal) {
        parameterParent->hover();
    }
}

void ParameterPort::mouseExit(const MouseEvent &event) {
    SelectHoverObject::mouseExit(event);

    if (isInternal) {
        parameterParent->hover(false);
    }
}


