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
    socket1.pos = getLocalPoint(startPort,
                      portCenter);
    socket2.pos = getLocalPoint(getParentComponent(),
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

bool Connection::connectEnd(ConnectionPort *endPort) {
    if (endPort != nullptr) {
        // Create connection
        port2 = endPort;

        // Update bounds
        auto portCenter = Point<int>(endPort->getWidth() / 2, endPort->getHeight() / 2);
        Rectangle<int> bounds(
                socket1.relativeToParentPos,
                getParentComponent()->getLocalPoint(endPort, portCenter));
        setBounds(bounds);

        // Update socket1 pos if bounds have changed
        socket1.pos = getLocalPoint(getParentComponent(), socket1.relativeToParentPos);
        // Update socket2 to dragPos
        socket2.pos = getLocalPoint(endPort, portCenter);
        socket2.relativeToParentPos = getParentComponent()->getLocalPoint(this,
                                                                          socket2.pos);

        return true;
    } else {
        // Cancel connection
        auto parent = dynamic_cast<SceneComponent*>(getParentComponent());
        parent->removeSceneComponent(this);

        return false;
    }

}

void Connection::paint(Graphics &g) {
    auto line = Line<float>(socket1.pos.toFloat(), socket2.pos.toFloat());
    g.setColour(Colours::lightgrey);
    g.drawLine(line, 4);
    g.setColour(Colours::darkgrey);
    g.drawLine(line, 2);
    
}

bool Connection::isConnectedTo(SceneComponent *componentToCheck) {
    if (componentToCheck == nullptr) return false;
    return port1->getParentComponent() == componentToCheck || port2->getParentComponent() == componentToCheck;
}

void Connection::updatePosition() {
    // Update bounds
    auto port1Center = Point<int>(port1->getWidth() / 2, port1->getHeight() / 2);
    auto port2Center = Point<int>(port2->getWidth() / 2, port2->getHeight() / 2);

    Rectangle<int> bounds(
            getParentComponent()->getLocalPoint(port1, port1Center),
            getParentComponent()->getLocalPoint(port2, port2Center));
    setBounds(bounds);

    // Update sockets pos based on new parent positions
    socket1.pos = getLocalPoint(port1, port1Center);
    socket2.pos = getLocalPoint(port2, port2Center);
    // Update sockets relative pos
    socket1.relativeToParentPos = getParentComponent()->getLocalPoint(this,
                                                                      socket1.pos);
    socket2.relativeToParentPos = getParentComponent()->getLocalPoint(this,
                                                                      socket2.pos);
}

void Connection::addListener(ComponentListener *listener) {
    if (port1 != nullptr) port1->getParentComponent()->addComponentListener(listener);
    if (port2 != nullptr) port2->getParentComponent()->addComponentListener(listener);
}

void Connection::removeListener(ComponentListener *listener) {
    if (port1 != nullptr) port1->getParentComponent()->removeComponentListener(listener);
    if (port2 != nullptr) port2->getParentComponent()->removeComponentListener(listener);
}
