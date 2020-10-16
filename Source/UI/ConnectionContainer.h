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

class ConnectionContainer
{
public:
    void addConnection(ConnectionPort* port1, ConnectionPort* port2);

    void startConnectionDrag(ConnectionPort* port1, Point<float> mousePos);
    void connectionDrag(Point<float> mousePos);
    void endConnectionDrag(Point<float> mousePos);

private:
    ReferenceCountedArray<Connection> connections;
};