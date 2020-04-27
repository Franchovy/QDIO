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


//==============================================================================
// Line Component methods


void LineComponent::convert(ConnectionPort *port2) {
    if (port1 != nullptr) {
        // Connect port1 to port2

    }
}



void LineComponent::paint(Graphics &g) {
    std::cout << "LineComponent paint" << newLine;

    g.setColour(Colours::whitesmoke);
    Path p;
    p.addLineSegment(line.toFloat(),1);
    PathStrokeType strokeType(1);

    float thiccness[] = {5, 5};
    strokeType.createDashedStroke(p, p, thiccness, 2);

    g.strokePath(p, strokeType);
}

void LineComponent::startDrag(ConnectionPort *p, const MouseEvent &event) {
    port1 = p;

    Component* parent = p->getDragLineParent();
    parent->addAndMakeVisible(this);

    auto newBounds = Rectangle<int>( parent->getLocalPoint(p, p->centrePoint),
            parent->getLocalPoint(event.eventComponent, event.getPosition()));
    setBounds(newBounds);

    p1 = getLocalPoint(p, p->centrePoint);
    p2 = getLocalPoint(event.eventComponent, event.getPosition());
    line.setStart(p1);
    line.setEnd(p2);

    repaint();
}

void LineComponent::drag(const MouseEvent &event) {
    auto newP2 = getLocalPoint(event.eventComponent, event.getPosition());
    auto newBounds = Rectangle<int>(p1, newP2) + getPosition();
    setBounds(newBounds);

    p1 = getLocalPoint(port1, port1->centrePoint);
    p2 = getLocalPoint(event.eventComponent, event.getPosition());
    line.setStart(p1);
    line.setEnd(p2);

    repaint();
}

void LineComponent::release(ConnectionPort *port2) {
    /*if (port2 != nullptr)
    {
        auto newP2 = getLocalPoint(port2, port2->centrePoint);
        auto newBounds = Rectangle<int>(p1, newP2) + getPosition();
        setBounds(newBounds);

        p1 = getLocalPoint(port1, port1->centrePoint);
        p2 = getLocalPoint(port2, port2->centrePoint);
        line.setStart(p1);
        line.setEnd(p2);

        repaint();
    }*/
    // clear data
    port1 = nullptr;
    p1 = p2 = Point<int>();
    setVisible(false);
    repaint();
}

ConnectionPort *LineComponent::getPort1() {
    return port1;
}

void ConnectionLine::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    auto inPos = getParentComponent()->getLocalPoint(inPort.get(), inPort->centrePoint);
    auto outPos = getParentComponent()->getLocalPoint(outPort.get(), outPort->centrePoint);

    auto newBounds = Rectangle<int>(inPos, outPos);
    setBounds(newBounds);

    line.setStart(getLocalPoint(getParentComponent(), inPos));
    line.setEnd(getLocalPoint(getParentComponent(), outPos));

    repaint();
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


ConnectionLine::ConnectionLine(ConnectionPort &p1, ConnectionPort &p2) {
    // Remember that input ports are the line output and vice versa
    if (!p1.isInput) {
        inPort = &p1;
        outPort = &p2;
    } else {
        inPort = &p2;
        outPort = &p1;
    }

    auto inPos = dynamic_cast<InternalConnectionPort*>(inPort.get()) != nullptr
                 ? inPort->getPosition() + inPort->centrePoint
                 : inPort->getParentComponent()->getPosition() + inPort->getPosition() + inPort->centrePoint;
    auto outPos = dynamic_cast<InternalConnectionPort*>(outPort.get()) != nullptr
                  ? outPort->getPosition() + outPort->centrePoint
                  : outPort->getParentComponent()->getPosition() + outPort->getPosition() + outPort->centrePoint;

    auto newBounds = Rectangle<int>(inPos, outPos);
    setBounds(newBounds);

    line = Line<int>(getLocalPoint(getParentComponent(), inPos),
            getLocalPoint(getParentComponent(), outPos));

    inPort->setOtherPort(outPort);
    outPort->setOtherPort(inPort);
    inPort->getParentComponent()->addComponentListener(this);
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
