/*
  ==============================================================================

    Connection.cpp
    Created: 16 Oct 2020 3:03:49pm
    Author:  maxime

  ==============================================================================
*/

#include "Connection.h"

Connection::Connection() {

}

Connection::Connection(ConnectionPort *port1, ConnectionPort *port2) {
    this->port1 = port1;
    this->port2 = port2;
}

void Connection::connectStart(ConnectionPort *startPort, Point<int> dragPos) {
    auto portCenter = Point<int>(startPort->getWidth() / 2, startPort->getHeight() / 2);
    Rectangle<int> newBounds(getParentComponent()->getLocalPoint(startPort, portCenter),
            dragPos);
    setBounds(newBounds);

    port1 = startPort;
    socket1.pos = this->getLocalPoint(startPort,
                      portCenter);
    socket2.pos = this->getLocalPoint(getParentComponent(),
                                  dragPos);
    socket1.relativeToParentPos = getParentComponent()->getLocalPoint(startPort,
                                                     portCenter);
    socket2.relativeToParentPos = getParentComponent()->getLocalPoint(
            this, socket2.pos);
}

void Connection::connectDrag(Point<int> dragPos) {
    Rectangle<int> bounds(
            socket1.relativeToParentPos,
            dragPos);
    setBounds(bounds);

    // Update socket1 pos if bounds have changed
    socket1.pos = getLocalPoint(getParentComponent(), socket1.relativeToParentPos);
    // Update socket2 to dragPos
    socket2.pos = getLocalPoint(getParentComponent(), dragPos);
    socket2.relativeToParentPos = getParentComponent()->getLocalPoint(this,
            socket2.pos);
}

void Connection::connectEnd(ConnectionPort *endPort) {


}

void Connection::paint(Graphics &g) {
    auto line = Line<float>(socket1.pos.toFloat(), socket2.pos.toFloat());
    g.setColour(Colours::lightgrey);
    g.drawLine(line, 4);
    g.setColour(Colours::darkgrey);
    g.drawLine(line, 2);
    
}
