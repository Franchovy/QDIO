/*
  ==============================================================================

    ConnectionGraph.cpp
    Created: 1 Jul 2020 12:18:22pm
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionGraph.h"

#include "Effect.h"

ConnectionGraph* ConnectionGraph::instance = nullptr;

ConnectionGraph::ConnectionGraph(AudioProcessorGraph& audioGraph)
    : audioGraph(audioGraph)
{
    instance = this;
}

bool ConnectionGraph::addConnection(const ConnectionLine* line) {
    std::cout << "Add connection" << newLine;

    auto array = Array<AudioProcessorGraph::Connection>();

    auto inPort = line->getOutPort();
    auto outPort = line->getInPort();

    if (inPort == nullptr || outPort == nullptr) {
        return false;
    }

    auto inEffect = dynamic_cast<Effect *>(inPort->getParentComponent());
    auto outEffect = dynamic_cast<Effect *>(outPort->getParentComponent());

    if (inEffect == nullptr || outEffect == nullptr) {
        return false;
    }

    auto inEndPort = dynamic_cast<AudioPort*>(inEffect->getEndPort(inPort));
    auto outEndPort = dynamic_cast<AudioPort*>(outEffect->getEndPort(outPort));

    if (inEndPort != nullptr && outEndPort != nullptr) {
        auto inEndEffect = dynamic_cast<Effect*>(inEndPort->getParentEffect());
        auto outEndEffect = dynamic_cast<Effect*>(outEndPort->getParentEffect());
        
        if (! (inEndEffect->isIndividual() && outEndEffect->isIndividual())) {
            return false;
        }
        
        auto inNode = inEndEffect->getNodeID();
        auto outNode = outEndEffect->getNodeID();

        auto newConnection = std::make_unique<AudioConnection>();
        newConnection->input = inEndEffect;
        newConnection->output = outEndEffect;
        newConnection->connections.addArray(inEffect->getConnectionsUntilEnd(inPort));
        newConnection->connections.addArray(outEffect->getConnectionsUntilEnd(outPort));
        newConnection->validConnection = true;

        for (int c = 0; c < jmin(audioGraph.getTotalNumInputChannels(),
                                 audioGraph.getTotalNumOutputChannels()); c++) {
            AudioProcessorGraph::Connection connection = {{inNode, inEndPort->bus->getChannelIndexInProcessBlockBuffer(c) },
                                                          {outNode, outEndPort->bus->getChannelIndexInProcessBlockBuffer(c) }};

            newConnection->audioConnections.add(connection);
            audioGraph.addConnection(connection);
            //array.add(connection);
        }
        connections.add(std::move(newConnection));
        
        return true;
/*
        auto connection = array.getFirst();
        for (auto c = 0; c < 2; c++) { // force 2 channels
            connection.source.channelIndex = c;
            connection.destination.channelIndex = c;

            if (! audioGraph.isConnected(connection) &&
                audioGraph.isConnectionLegal(connection)) {
                std::cout << "add audio connection channel" << newLine;
                // Make audio connection
                audioGraph.addConnection(connection);
            }
        }
*/
        
    }
    return false;
}

void ConnectionGraph::removeConnection(const ConnectionLine* line) {
    std::cout << "Remove connection" << newLine;

    auto array = Array<AudioProcessorGraph::Connection>();

    auto inPort = line->getOutPort();
    auto outPort = line->getInPort();

    if (inPort == nullptr || outPort == nullptr) {
        return;
    }

    auto inEffect = dynamic_cast<Effect *>(inPort->getParentComponent());
    auto outEffect = dynamic_cast<Effect *>(outPort->getParentComponent());

    if (inEffect == nullptr || outEffect == nullptr) {
        return;
    }

    auto inEndPort = dynamic_cast<AudioPort*>(inEffect->getEndPort(inPort));
    auto outEndPort = dynamic_cast<AudioPort*>(outEffect->getEndPort(outPort));

    if (inEndPort != nullptr && outEndPort != nullptr) {
        // Look for connection match
        for (auto c : connections) {
            if (c->input == inEndPort->getParentEffect() && c->output == outEndPort->getParentEffect()) {
                // Remove audio connections and //todo invalidate the struct.
                for (auto audioConnection : c->audioConnections)
                {
                    audioGraph.removeConnection(audioConnection);
                }
                connections.removeObject(c, true);
                break;
            }
        }

        /*for (int c = 0; c < jmin(audioGraph.getTotalNumInputChannels(),
                                 audioGraph.getTotalNumOutputChannels()); c++) {
            AudioProcessorGraph::Connection connection = {{inNode,  inEndPort->bus->getChannelIndexInProcessBlockBuffer(
                    c)},
                                                          {outNode, outEndPort->bus->getChannelIndexInProcessBlockBuffer(
                                                                  c)}};
            array.add(connection);
        }*/
    }

/*    for (auto connection : array) {
        if (audioGraph.isConnected(connection)) {
            std::cout << "remove audio connection channel" << newLine;
            audioGraph.removeConnection(connection);
        }
    }*/

}

void ConnectionGraph::updateNumChannels(int numChannels) {
    std::cout << "update num channels" << newLine;

}

/*void ConnectionGraph::componentParentHierarchyChanged(Component &component) {
    if (auto effect = dynamic_cast<Effect*>(&component)) {
        if (effect->getParentComponent() == nullptr) {
            // Remove from audiograph

        } else {
            // Add to audiograph
            //audioGraph.addNode(effect->getProcessor());
            auto processor = audioGraph.getNodeForId(effect->getNodeID())->getProcessor();

            processor->setPlayConfigDetails(
                    processor->getTotalNumInputChannels(),
                    processor->getTotalNumOutputChannels(),
                    audioGraph.getSampleRate(),
                    audioGraph.getBlockSize());
        }
    } else if (auto connectionLine = dynamic_cast<ConnectionLine*>(&component)) {
        if (connectionLine->getParentComponent() == nullptr) {
            // Disconnect
            removeConnection(connectionLine);
        } else {
            // Connect
            addConnection(connectionLine);
        }
    }

    ComponentListener::componentParentHierarchyChanged(component);
}*/

/*void ConnectionGraph::componentBeingDeleted(Component &component) {
    if (auto effect = dynamic_cast<Effect*>(&component)) {
        audioGraph.removeNode(effect->getNodeID());
    }

    ComponentListener::componentBeingDeleted(component);
}*/

AudioProcessorGraph::Node *ConnectionGraph::addNode(std::unique_ptr<AudioProcessor> newProcessor) {
    return audioGraph.addNode(move(newProcessor)).get();
}



AudioConnection::AudioConnection() {

}
