/*
  ==============================================================================

    ConnectionGraph.cpp
    Created: 1 Jul 2020 12:18:22pm
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionGraph.h"

#include "Effect.h"

ConnectionGraph::ConnectionGraph(AudioProcessorGraph& audioGraph)
    : audioGraph(audioGraph)
{

}

void ConnectionGraph::addConnection(const ConnectionLine& line) {
    std::cout << "Add connection" << newLine;

    Effect::NodeAndPort in;
    Effect::NodeAndPort out;

    auto array = Array<AudioProcessorGraph::Connection>();

    auto inPort = line.getOutPort();
    auto outPort = line.getInPort();

    if (inPort == nullptr || outPort == nullptr) {
        return;
    }

    auto inEffect = dynamic_cast<Effect *>(inPort->getParentComponent());
    auto outEffect = dynamic_cast<Effect *>(outPort->getParentComponent());

    if (inEffect == nullptr || outEffect == nullptr) {
        return;
    }

    in = inEffect->getNode(inPort);
    out = outEffect->getNode(outPort);

// todo change to using audioConnection struct
    if (in.isValid && out.isValid) {
        for (int c = 0; c < jmin(audioGraph.getTotalNumInputChannels(),
                                 audioGraph.getTotalNumOutputChannels()); c++) {
            AudioProcessorGraph::Connection connection = {{in.node->nodeID,  in.port->bus->getChannelIndexInProcessBlockBuffer(
                    c)},
                                                          {out.node->nodeID, out.port->bus->getChannelIndexInProcessBlockBuffer(
                                                                  c)}};
            array.add(connection);
        }
    }


    //==========================================================



    //==========================================================

    if (array.isEmpty()) {
        return;
    }

    auto connection = array.getFirst();
    for (auto c = 0; c < 2; c++) { // default 2 channels
        connection.source.channelIndex = c;
        connection.destination.channelIndex = c;

        if (! audioGraph.isConnected(connection) &&
            audioGraph.isConnectionLegal(connection)) {
            // Make audio connection
            audioGraph.addConnection(connection);
        }
    }

}

void ConnectionGraph::removeConnection(const ConnectionLine& line) {
    std::cout << "Remove connection" << newLine;

    Effect::NodeAndPort in;
    Effect::NodeAndPort out;

    auto array = Array<AudioProcessorGraph::Connection>();

    auto inPort = line.getOutPort();
    auto outPort = line.getInPort();

    if (inPort == nullptr || outPort == nullptr) {
        return;
    }

    auto inEffect = dynamic_cast<Effect *>(inPort->getParentComponent());
    auto outEffect = dynamic_cast<Effect *>(outPort->getParentComponent());

    if (inEffect == nullptr || outEffect == nullptr) {
        return;
    }

    in = inEffect->getNode(inPort);
    out = outEffect->getNode(outPort);

// todo change to using audioConnection struct
    if (in.isValid && out.isValid) {
        for (int c = 0; c < jmin(audioGraph.getTotalNumInputChannels(),
                                 audioGraph.getTotalNumOutputChannels()); c++) {
            AudioProcessorGraph::Connection connection = {{in.node->nodeID,  in.port->bus->getChannelIndexInProcessBlockBuffer(
                    c)},
                                                          {out.node->nodeID, out.port->bus->getChannelIndexInProcessBlockBuffer(
                                                                  c)}};
            array.add(connection);
        }
    }

    for (auto connection : array) {
        if (audioGraph.isConnected(connection)) {
            audioGraph.removeConnection(connection);
        }
    }

}

void ConnectionGraph::updateNumChannels(int numChannels) {
    std::cout << "update num channels" << newLine;

}
