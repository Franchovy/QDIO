/*
  ==============================================================================

    ConnectionGraph.cpp
    Created: 1 Jul 2020 12:18:22pm
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionGraph.h"

#include "Effect.h"

ConnectionGraph::ConnectionGraph(AudioProcessorGraph& processorGraph)
    : audioGraph(processorGraph)
{

}

void ConnectionGraph::addConnection(const ConnectionLine& line) {
    std::cout << "Add connection" << newLine;
}

void ConnectionGraph::removeConnection(const ConnectionLine& line) {
    std::cout << "Remove connection" << newLine;
}

void ConnectionGraph::updateNumChannels(int numChannels) {
    std::cout << "update num channels" << newLine;
}
