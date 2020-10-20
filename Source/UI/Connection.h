/*
  ==============================================================================

    Connection.h
    Created: 16 Oct 2020 3:03:49pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SceneComponent.h"
#include "ConnectionPort.h"

class ConnectionPlug : public SceneComponent
{
public:
    Point<int> relativeToParentPos;
    Point<int> pos;
private:

};

class Connection : public SceneComponent
{
public:
    Connection();
    Connection(ConnectionPort* port1, ConnectionPort* port2);

    void connectStart(ConnectionPort* startPort, Point<int> dragPos);
    void connectDrag(Point<int> dragPos);
    bool connectEnd(ConnectionPort* endPort);

    void addListener(ComponentListener* listener);
    void removeListener(ComponentListener* listener);

    void paint(Graphics &g) override;

    bool isConnectedTo(SceneComponent* componentToCheck);
    void updatePosition();

private:
    ConnectionPort* port1 = nullptr;
    ConnectionPort* port2 = nullptr;
    ConnectionPlug socket1;
    ConnectionPlug socket2;
};