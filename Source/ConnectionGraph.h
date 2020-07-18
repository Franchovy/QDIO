/*
  ==============================================================================

    ConnectionGraph.h
    Created: 1 Jul 2020 12:18:22pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Ports.h"

class Effect;
class ConnectionLine;

struct AudioConnection
{
    AudioConnection();

    Effect* input = nullptr;
    Effect* output = nullptr;
    Array<ConnectionLine*> connections;
    Array<AudioProcessorGraph::Connection> audioConnections;
    bool validConnection = false;
};

class ConnectionGraph : public ComponentListener
{
public:
    ConnectionGraph(AudioProcessorGraph& audioGraph);

    static ConnectionGraph* getInstance() { return instance; }

    void componentParentHierarchyChanged(Component &component) override;

    bool addConnection(const ConnectionLine* line);
    void removeConnection(const ConnectionLine* line);

    void updateNumChannels(int numChannels);

private:
    static ConnectionGraph* instance;

    AudioProcessorGraph& audioGraph;
    OwnedArray<AudioConnection> connections;
    HashMap<const ConnectionLine*, const AudioConnection*> lineToAudioConnectionsMap;
};
