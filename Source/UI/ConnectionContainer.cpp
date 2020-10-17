/*
  ==============================================================================

    ConnectionContainer.cpp
    Created: 15 Oct 2020 11:16:43am
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionContainer.h"

void ConnectionContainer::addConnection(ConnectionPort *port1, ConnectionPort *port2) {


}

void ConnectionContainer::startConnectionDrag(Component* thisComponent, ConnectionPort *port1, Point<float> mousePos) {
    auto connection = new Connection();
    thisComponent->addAndMakeVisible(connection);
    connection->connectStart(port1, mousePos.toInt());
    connections.add(connection);
    activeConnection = connection;
}

void ConnectionContainer::connectionDrag(Point<float> mousePos) {
    activeConnection->connectDrag(mousePos.toInt());
}

void ConnectionContainer::endConnectionDrag(ConnectionPort* port2) {
    activeConnection->connectEnd(port2);
}
