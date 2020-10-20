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
    if (activeConnection->connectEnd(port2)) {
        // Connection was successful
        activeConnection->addListener(this);
    } else {
        // Connection unsuccessful, got cancelled
        activeConnection->removeListener(this);
    }

}

void ConnectionContainer::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    for (auto c : connections) {
        if (c->isConnectedTo(dynamic_cast<SceneComponent*>(&component))) {
            c->updatePosition();
        }
    }
    ComponentListener::componentMovedOrResized(component, wasMoved, wasResized);
}
