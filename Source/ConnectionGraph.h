/*
  ==============================================================================

    ConnectionGraph.h
    Created: 1 Jul 2020 12:18:22pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Effect;
class ConnectionLine;

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

    void addConnection(const ConnectionLine& line);
    void removeConnection(const ConnectionLine& line);

    void updateNumChannels(int numChannels);

private:
    AudioProcessorGraph& audioGraph;
    Array<AudioConnection> connections;
};