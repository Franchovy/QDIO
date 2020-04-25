/*
  ==============================================================================

    Ports.cpp
    Created: 25 Apr 2020 11:41:27am
    Author:  maxime

  ==============================================================================
*/

#include "Ports.h"


AudioPort::AudioPort(bool isInput) : ConnectionPort()
        , internalPort(new InternalConnectionPort(this, !isInput))
{

    hoverBox = Rectangle<int>(0,0,60,60);
    outline = Rectangle<int>(20, 20, 20, 20);
    centrePoint = Point<int>(30,30);
    setBounds(0,0,60, 60);

    this->isInput = isInput;
}

bool AudioPort::canConnect(ConnectionPort::Ptr& other) {
    if (this->isInput == other->isInput)
        return false;

    // Connect to AudioPort of mutual parent
    return (dynamic_cast<AudioPort *>(other.get())
            && other->getParentComponent()->getParentComponent() == this->getParentComponent()->getParentComponent())
           // Connect to ICP of containing parent effect
           || (dynamic_cast<InternalConnectionPort *>(other.get())
               && other->getParentComponent() == this->getParentComponent()->getParentComponent());
}


void ConnectionPort::paint(Graphics &g) {
    g.setColour(findColour(ColourIDs::portColour));
    //rectangle.setPosition(10,10);
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
};

bool InternalConnectionPort::canConnect(ConnectionPort::Ptr& other) {
    // Return false if the port is AP and belongs to the same parent
    return !(dynamic_cast<AudioPort *>(other.get())
             && this->getParentComponent() == other->getParentComponent());
}

InternalConnectionPort::InternalConnectionPort(AudioPort *parent, bool isInput) : ConnectionPort() {
    audioPort = parent;
    this->isInput = isInput;

    hoverBox = Rectangle<int>(0,0,30,30);
    outline = Rectangle<int>(10,10,10,10);
    centrePoint = Point<int>(15,15);
    //setBounds(parent->getX(), parent->getY(), 30, 30);
    setBounds(0, 0, 30, 30);
}