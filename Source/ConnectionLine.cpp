/*
  ==============================================================================

    ConnectionLine.cpp
    Created: 24 Apr 2020 10:45:08am
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionLine.h"


LineComponent* LineComponent::dragLine = nullptr;

const Identifier ConnectionLine::IDs::CONNECTION_ID = "connection";
const Identifier ConnectionLine::IDs::ConnectionLineObject = "connectionLineObject";
const Identifier ConnectionLine::IDs::InPort = "inport";
const Identifier ConnectionLine::IDs::OutPort = "outport";
const Identifier ConnectionLine::IDs::AudioConnection = "audioConnection";


//==============================================================================
// Line Component methods

/**
 * @param event modified mouseEvent to use mainComponent coords.
 */
void LineComponent::mouseDown(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);

    if (port1 = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        p1 = getLocalPoint(port1, port1->centrePoint);
    }

    p2 = thisEvent.getPosition();

    line.setStart(p1);
    line.setEnd(p2);

    setVisible(true);
    repaint();

    getParentComponent()->mouseDown(event);
}

void LineComponent::mouseDrag(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);
    p2 = thisEvent.getPosition();

    line.setEnd(p2);
    repaint();

    // Pass hover detection to EffectScene
    getParentComponent()->mouseDrag(thisEvent);
}

void LineComponent::mouseUp(const MouseEvent &event) {
    setVisible(false);

    // Pass this event to EffectScene
    auto eventMain = event.getEventRelativeTo(this).withNewPosition(
            event.getPosition() - getPosition()
    );
    getParentComponent()->mouseUp(eventMain);
}

void LineComponent::convert(ConnectionPort *port2) {
    if (port1 != nullptr) {
        // Connect port1 to port2

    }
}

LineComponent::LineComponent() {

}

void LineComponent::resized() {
    Component::resized();
}

void LineComponent::paint(Graphics &g) {
    g.setColour(Colours::navajowhite);
    Path p;
    p.addLineSegment(line.toFloat(),1);
    PathStrokeType strokeType(1);

    float thiccness[] = {5, 5};
    strokeType.createDashedStroke(p, p, thiccness, 2);

    g.strokePath(p, strokeType);

    Component::paint(g);
}

LineComponent *LineComponent::getDragLine() {
    return dragLine;
}

void LineComponent::setDragLine(LineComponent *newLine) {
    dragLine = newLine;
}

void ConnectionLine::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    //setBounds(Rectangle<int>(line.getStart(), line.getEnd()));

    line.setStart(getLocalPoint(inPort.get(), inPort->centrePoint));
    line.setEnd(getLocalPoint(outPort.get(), outPort->centrePoint));

    repaint();
}

bool ConnectionLine::hitTest(int x, int y) {
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

    auto inPos = dynamic_cast<AudioPort*>(inPort.get()) != nullptr
                 ? inPort->getParentComponent()->getPosition() + inPort->getPosition() + inPort->centrePoint
                 : inPort->getPosition() + inPort->centrePoint;
    auto outPos = dynamic_cast<AudioPort*>(outPort.get()) != nullptr
                  ? outPort->getParentComponent()->getPosition() + outPort->getPosition() + outPort->centrePoint
                  : outPort->getPosition() + outPort->centrePoint;


    line = Line<int>(inPos, outPos);

    inPort->setOtherPort(outPort);
    outPort->setOtherPort(inPort);
    inPort->getParentComponent()->addComponentListener(this);
    outPort->getParentComponent()->addComponentListener(this);


    setBounds(0, 0, getParentWidth()*2, getParentHeight()*2);
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
    int thiccness;
    if (hoverMode || selectMode) {
        g.setColour(Colours::blue);
        thiccness = 3;
    } else {
        g.setColour(Colours::whitesmoke);
        thiccness = 2;
    }

    g.drawLine(line.toFloat(),thiccness);
}


void ConnectionPort::mouseDown(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDown(event);
}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDrag(event);
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseUp(event);
}
