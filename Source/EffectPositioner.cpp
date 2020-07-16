/*
  ==============================================================================

    EffectPositioner.cpp
    Created: 6 Jul 2020 10:52:30am
    Author:  maxime

  ==============================================================================
*/

#include "EffectPositioner.h"

EffectPositioner* EffectPositioner::instance = nullptr;


void EffectPositioner::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    // Avoid a recursive mess!
    if (movingOp)
        return;

    auto effect = dynamic_cast<Effect*>(&component);
    jassert(effect != nullptr);

    if (wasMoved) {
        // if connected
            // effect center -> get connection on each side
                // check distance
                    // disconnect
        // if not connected
            // check connections -> hittest effect center
                // connect
        // if connected
            // if effect center is further than next effect center
                // switch over effects:
                // reassign connections
                // move effect2 into correct place
    }

    if (wasResized) {
        for (auto c : effect->getConnectionsToThis()) {
            if (c->type == ConnectionLine::audio) {
                // get full end connection
                for (auto port : effect->getPorts()) {
                    if (port->isConnected()) {

                        Array<Effect*> connectedEffects;
                        bool rightWard = ! port->isInput;

                        connectedEffects.add(effect);
                        connectedEffects.addArray(effect->getFullConnectionEffects(port)); //todo replace port with direction bool

                        Effect* leftEffect = nullptr;
                        Effect* rightEffect = nullptr;

                        for (auto i = 1; i < connectedEffects.size(); i++) {
                            // Set the left and right effects to check position for
                            leftEffect = rightWard ? connectedEffects[i - 1] : connectedEffects[i];
                            rightEffect = rightWard ? connectedEffects[i] : connectedEffects[i - 1]; //fixme these set backwards?

                            int fitDistance = getFittedDistance(leftEffect, rightEffect);

                            if (fitDistance < 0) {
                                movingOp = true;
                                // Scooch the effects
                                moveEffect(connectedEffects[i], -fitDistance, rightWard); //fixme flip distance

                                movingOp = false;
                            }
                        }
                    }
                }
            }
        }
    }

    ComponentListener::componentMovedOrResized(component, wasMoved, wasResized);
}

EffectPositioner::EffectPositioner() {
    instance = this;
}

EffectPositioner *EffectPositioner::getInstance() {
    return instance;
}

int EffectPositioner::getFittedDistance(Effect *leftEffect, Effect *rightEffect) {
    return rightEffect->getX() - minDistanceBetweenEffects - leftEffect->getRight();
}

void EffectPositioner::moveEffect(Effect* effect, int distance, bool rightWard) {
    if (rightWard) {
        effect->setTopLeftPosition(effect->getX() + distance, effect->getY());
    } else {
        effect->setTopLeftPosition(effect->getX() - distance, effect->getY());
    }
}


void EffectPositioner::mergeConnection(ConnectionLine *inLine, ConnectionLine *outLine) {
    //jassert(getPorts(true).contains(inLine->getInPort().get()));
    //jassert(getPorts(false).contains(outLine->getOutPort().get()));

    auto newInPort = outLine->getInPort();
    outLine->getParentComponent()->removeChildComponent(outLine);

    inLine->unsetPort(inLine->getInPort());
    inLine->setPort(newInPort);
}

void EffectPositioner::extendConnection(ConnectionLine *lineToExtend, Effect *parentToExtendThrough) {
    if (lineToExtend->getInPort()->getDragLineParent() == parentToExtendThrough
        || lineToExtend->getOutPort()->getDragLineParent() == parentToExtendThrough)
    {
        // Exit parent
        bool isInput = parentToExtendThrough->isParentOf(lineToExtend->getInPort());

        auto newPort = parentToExtendThrough->addPort(Effect::getDefaultBus(), isInput);
        auto oldPort = isInput ? lineToExtend->getInPort() : lineToExtend->getOutPort();

        // Set line ports
        lineToExtend->unsetPort(oldPort);
        parentToExtendThrough->getParentComponent()->addAndMakeVisible(lineToExtend);
        lineToExtend->setPort(newPort.get());

        // Create new internal line
        auto newConnection = new ConnectionLine();
        parentToExtendThrough->addAndMakeVisible(newConnection);
        parentToExtendThrough->resized();
        newConnection->setPort(oldPort);
        newConnection->setPort(newPort->internalPort.get());
    } else {
        // Enter parent
        auto isInput = lineToExtend->getInPort()->getDragLineParent() == parentToExtendThrough;
        auto oldPort = isInput ? lineToExtend->getInPort() : lineToExtend->getOutPort();

        auto newPort = parentToExtendThrough->addPort(Effect::getDefaultBus(), isInput);
        lineToExtend->unsetPort(oldPort);
        lineToExtend->setPort(newPort.get());

        auto newConnection = new ConnectionLine();
        parentToExtendThrough->addAndMakeVisible(newConnection);
        newConnection->setPort(oldPort);
        newConnection->setPort(newPort->internalPort.get());
    }
}

void EffectPositioner::shortenConnection(ConnectionLine *interiorLine, ConnectionLine *exteriorLine) {
    auto parent = dynamic_cast<EffectBase*>(exteriorLine->getParentComponent());
    auto targetEffect = dynamic_cast<Effect*>(interiorLine->getParentComponent());

    jassert(targetEffect->getParentComponent() == parent);

    // Check which port has moved
    if (interiorLine->getInPort()->getDragLineParent() != targetEffect
        || interiorLine->getOutPort()->getDragLineParent() != targetEffect)
    {
        // interior line to remove
        auto portToRemove = interiorLine->getInPort()->getParentComponent() == targetEffect
                            ? interiorLine->getInPort() : interiorLine->getOutPort();
        auto newPort = interiorLine->getInPort()->getParentComponent() == targetEffect
                       ? interiorLine->getOutPort() : interiorLine->getInPort();
        targetEffect->removeChildComponent(interiorLine);

        exteriorLine->unsetPort(portToRemove->getLinkedPort());
        exteriorLine->setPort(newPort);

        // Remove unused port
        targetEffect->removePort(portToRemove);
    } else {
        // exterior line to remove
        auto portToRemove = exteriorLine->getInPort()->getParentComponent() == targetEffect
                            ? exteriorLine->getInPort() : exteriorLine->getOutPort();
        auto portToReconnect = exteriorLine->getInPort()->getParentComponent() == targetEffect
                               ? exteriorLine->getOutPort() : exteriorLine->getInPort();
        targetEffect->removeChildComponent(exteriorLine);

        interiorLine->unsetPort(portToRemove->getLinkedPort());
        interiorLine->setPort(portToReconnect);

        // Remove line
        parent->removeChildComponent(exteriorLine);

        // Remove unused port
        targetEffect->removePort(portToRemove);
    }
}
