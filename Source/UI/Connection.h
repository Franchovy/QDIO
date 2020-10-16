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

private:

};

class Connection : public SceneComponent
{
public:
    Connection();
    Connection(ConnectionPort* inPort, ConnectionPort* outPort);

private:
    void paint(Graphics &g) override;

public:

    void connectStart(ConnectionPort* start);
    void connectDrag(Point<int> dragPos);
    void connectEnd(ConnectionPort* end);

private:
    ConnectionPort* inPort = nullptr;
    ConnectionPort* outPort = nullptr;
    Point<float> inSocketPosition;
    Point<float> outSocketPosition;
};