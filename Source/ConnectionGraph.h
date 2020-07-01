/*
  ==============================================================================

    ConnectionGraph.h
    Created: 1 Jul 2020 12:18:22pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Effect.h"

struct AudioConnection
{
    Effect* input;
    Effect* output;
    Array<ConnectionLine*> connections;
    Array<AudioProcessorGraph::Connection> audioConnections;
};

class ConnectionGraph : public ComponentListener
{
public:
    ConnectionGraph(AudioProcessorGraph& processorGraph);

    void addConnection(ConnectionLine* line);
    void removeConnection(ConnectionLine* line);

    void updateNumChannels(int numChannels);

private:
    AudioProcessorGraph& audioGraph;
    Array<AudioConnection> connections;
};