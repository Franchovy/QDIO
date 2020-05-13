/*
  ==============================================================================

    ConnectionLine.cpp
    Created: 24 Apr 2020 10:45:08am
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionLine.h"



const Identifier ConnectionLine::IDs::CONNECTION_ID = "connection";
const Identifier ConnectionLine::IDs::ConnectionLineObject = "connectionLineObject";
const Identifier ConnectionLine::IDs::InPort = "inport";
const Identifier ConnectionLine::IDs::OutPort = "outport";
const Identifier ConnectionLine::IDs::AudioConnection = "audioConnection";



ConnectionLine::ConnectionLine() {
    inPort = nullptr;
    outPort = nullptr;
}

void ConnectionLine::setInPort(ConnectionPort *port) {
    jassert(port->isInput);
    inPort = port;

    inPos = dynamic_cast<InternalConnectionPort*>(inPort.get()) != nullptr
                 ? inPort->getPosition() + inPort->centrePoint
                 : inPort->getParentComponent()->getPosition() + inPort->getPosition() + inPort->centrePoint;

    auto newBounds = Rectangle<int>(inPos, outPos);
    setBounds(newBounds);

    line = Line<int>(getLocalPoint(getParentComponent(), inPos),
                     getLocalPoint(getParentComponent(), outPos));

    inPort->setOtherPort(outPort);
    inPort->getParentComponent()->addComponentListener(this);
}

void ConnectionLine::setOutPort(ConnectionPort *port) {
    jassert(! port->isInput);
    outPort = port;

    outPos = dynamic_cast<InternalConnectionPort*>(outPort.get()) != nullptr
                  ? outPort->getPosition() + outPort->centrePoint
                  : outPort->getParentComponent()->getPosition() + outPort->getPosition() + outPort->centrePoint;

    auto newBounds = Rectangle<int>(inPos, outPos);
    setBounds(newBounds);

    line = Line<int>(getLocalPoint(getParentComponent(), inPos),
                     getLocalPoint(getParentComponent(), outPos));

    outPort->setOtherPort(inPort);
    outPort->getParentComponent()->addComponentListener(this);
}


void ConnectionLine::componentParentHierarchyChanged(Component &component) {
    ComponentListener::componentParentHierarchyChanged(component);
}

ConnectionLine::~ConnectionLine() {
    inPort->setOtherPort(nullptr);
    outPort->setOtherPort(nullptr);
    inPort->removeComponentListener(this);
    outPort->removeComponentListener(this);
}

void ConnectionLine::paint(Graphics &g) {
    Path p;
    int thiccness;

    if (hoverMode || selectMode) {
        g.setColour(Colours::blue);
        thiccness = 3;
    } else {
        g.setColour(Colours::whitesmoke);
        thiccness = 2;
    }

    p.addLineSegment(line.toFloat(),thiccness);

    g.fillPath(p);
}


void ConnectionLine::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    auto inPos = getParentComponent()->getLocalPoint(inPort.get(), inPort->centrePoint);
    auto outPos = getParentComponent()->getLocalPoint(outPort.get(), outPort->centrePoint);

    auto newBounds = Rectangle<int>(inPos, outPos);
    setBounds(newBounds);

    line.setStart(getLocalPoint(getParentComponent(), inPos));
    line.setEnd(getLocalPoint(getParentComponent(), outPos));

    //todo unecessary?
    //repaint();
}

bool ConnectionLine::hitTest(int x, int y) {
    bool i,j;
    getInterceptsMouseClicks(i, j);
    if (! i) {
        return false;
    }

    auto d1 = line.getStart().getDistanceFrom(Point<int>(x,y));
    auto d2 = line.getEnd().getDistanceFrom(Point<int>(x,y));
    auto d = d1 + d2 - line.getLength();

    if (d < 3) {
        return true;
    } else {
        return false;
    }
}


void ConnectionLine::mouseDown(const MouseEvent &event) {
    jassert (inPort != nullptr || outPort != nullptr);
    ConnectionPort* port = (inPort != nullptr) ? inPort.get() : outPort.get();

    auto parent = getParentComponent();
    auto newBounds = Rectangle<int>( parent->getLocalPoint(port, port->centrePoint),
                                     parent->getLocalPoint(event.eventComponent, event.getPosition()));
    setBounds(newBounds);

    if (port->isInput) {
        inPos = getLocalPoint(inPort.get(), port->centrePoint);
        outPos = getLocalPoint(event.eventComponent, event.getPosition());
    } else {
        outPos = getLocalPoint(inPort.get(), port->centrePoint);
        inPos = getLocalPoint(event.eventComponent, event.getPosition());
    }

    line.setStart(inPos);
    line.setEnd(outPos);

    SelectHoverObject::resetHoverObject();
}

void ConnectionLine::mouseDrag(const MouseEvent &event) {
    ConnectionPort* port = (inPort != nullptr) ? inPort.get() : outPort.get();
    Point<int>* pointToSet = (inPort != nullptr) ? &outPos : &inPos;
    *pointToSet = getLocalPoint(event.eventComponent, event.getPosition());

    auto newBounds = Rectangle<int>(inPos, outPos) + getPosition();
    setBounds(newBounds);

    /*p1 = getLocalPoint(port1, port1->centrePoint);
    p2 = getLocalPoint(event.eventComponent, event.getPosition());
    line.setStart(p1);
    line.setEnd(p2);
*/}

void ConnectionLine::mouseUp(const MouseEvent &event) {
    if (inPort != nullptr && outPort != nullptr) {
        inPos = getLocalPoint(inPort.get(), inPort->centrePoint);
        outPos = getLocalPoint(outPort.get(), outPort->centrePoint);
        line.setStart(inPos);
        line.setEnd(outPos);
    } else {
        // Cancel drag
        setVisible(false);
    }
}

void ConnectionLine::setDragPort(ConnectionPort *port) {
    jassert(inPort != nullptr || outPort != nullptr);

    if (inPort == nullptr) inPort = port;
    if (outPort == nullptr) outPort = port;
}

