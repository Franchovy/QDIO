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

Connection::Connection(ConnectionPort *inPort, ConnectionPort *outPort) {
    this->inPort = inPort;
    this->outPort = outPort;
}

void Connection::connectStart(ConnectionPort *start) {
    inPort = start;
    inSocketPosition = this->getLocalPoint(inPort,
            Point<int>(
                    inPort->getX() + inPort->getWidth(),
                    inPort->getY() + inPort->getHeight()
                    )
                ).toFloat();
}

void Connection::connectDrag(Point<int> dragPos) {

}

void Connection::connectEnd(ConnectionPort *end) {

}

void Connection::paint(Graphics &g) {
    g.drawLine(Line<float>(inSocketPosition, outSocketPosition), 2);

    SceneComponent::paint(g);
}
