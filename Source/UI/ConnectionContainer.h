/*
  ==============================================================================

    ConnectionContainer.h
    Created: 15 Oct 2020 11:16:43am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ConnectionPort.h"
#include "Connection.h"

class ConnectionContainer : public ComponentListener
{
public:
    void addConnection(ConnectionPort* port1, ConnectionPort* port2);

    void startConnectionDrag(Component* thisComponent, ConnectionPort* port1, Point<float> mousePos);
    void connectionDrag(Point<float> mousePos);
    void endConnectionDrag(ConnectionPort* port2);

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;

private:
    Connection* activeConnection = nullptr;
    ReferenceCountedArray<Connection> connections;
};