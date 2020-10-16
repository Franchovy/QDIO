/*
  ==============================================================================

    EffectPositioner.cpp
    Created: 6 Jul 2020 10:52:30am
    Author:  maxime

  ==============================================================================
*/

#include "EffectPositioner.h"

#include "Effect.h"

EffectPositioner* EffectPositioner::instance = nullptr;


void EffectPositioner::effectResized(Effect *effect) {
    // Avoid a recursive mess!
    if (movingOp || ! positionerRunning)
        return;

    auto parent = dynamic_cast<EffectBase*>(effect->getParentComponent());

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

                        fitEffects(leftEffect, rightEffect, rightWard);
                    }
                }
            }
        }
    }
}

void EffectPositioner::effectMoved(Effect *effect) {
// Avoid a recursive mess!
    if (movingOp || ! positionerRunning)
        return;

    auto parent = dynamic_cast<EffectBase*>(effect->getParentComponent());

    auto inputConnections = effect->getConnectionsToThis(true, ConnectionLine::audio);
    auto outputConnections = effect->getConnectionsToThis(false, ConnectionLine::audio);
    if (inputConnections.size() > 0 && outputConnections.size() > 0) {
        // Connections on both sides
        auto inputPortPosition = parent->getLocalPoint(inputConnections.getFirst()->getOutPort()
                , inputConnections.getFirst()->getOutPort()->centrePoint);
        auto outputPortPosition = parent->getLocalPoint(outputConnections.getFirst()->getInPort()
                , outputConnections.getFirst()->getInPort()->centrePoint);

        auto effectCenterPosition = getEffectCenter(effect);
        auto connectionCenterLine = Line<int>(inputPortPosition, outputPortPosition);

        auto connectionsLeft = effect->getFullConnectionEffects(effect->getPorts(true).getFirst());
        auto connectionsRight = effect->getFullConnectionEffects(effect->getPorts(false).getFirst());

        //fixme: Parent-Meta-Effects count as a connection! oops!

        if (connectionsLeft.size() == 0 || connectionsRight.size() == 0) {
            return;
        }

        if (connectionsLeft.getFirst()->getPorts(false).size() == 0
            || connectionsRight.getFirst()->getPorts(true).size() == 0)
        {
            // Scooch

        } else {
            if (getEffectCenter(connectionsLeft.getFirst()).getX() > getEffectCenter(effect).getX()) {
                // Effect overlaps next towards the left
                movingOp = true;
                swapEffects(effect, connectionsLeft.getFirst());
                movingOp = false;
            } else if (getEffectCenter(connectionsRight.getFirst()).getX() < getEffectCenter(effect).getX()) {
                // Effect overlaps next towards the right
                movingOp = true;
                swapEffects(effect, connectionsRight.getFirst());
                movingOp = false;
            } else if (getDistanceFromLineExtended(connectionCenterLine, effectCenterPosition) > 50) {
                removeEffectConnections(effect);
            }
        }
    } else if (inputConnections.size() > 0 || outputConnections.size() > 0) {
        // Connections on one side only
        // todo drag whole connection
    } else {
        // Not connected
        for (auto connectionLine : parent->getConnectionsInside()) {
            auto effectCenterPosition = effect->getPosition() + Point<int>(effect->getWidth(), effect->getHeight()) / 2;
            auto line = connectionLine->getLine();

            if (getDistanceFromLineExtended(line, effectCenterPosition) < 10) {
                insertEffect(effect, connectionLine);
                // todo make room on left, and right
                movingOp = true;
                makeRoomForEffect(effect);
                movingOp = false;
            }
        }
    }
}

void EffectPositioner::effectParentReassigned(Effect *effect, EffectBase *parent) {
    movingOp = true;
    effect->setCentrePosition(parent->getLocalPoint(scene, getEffectCenter(effect)));
    parent->addAndMakeVisible(effect);
    if (auto parentEffect = dynamic_cast<Effect*>(parent)) {

        // Add ports to parent if it doesn't have any
        auto parentPorts = parentEffect->getPorts(-1);
        if (parentPorts.size() == 0) {
            // Check what ports new child has
            auto numInputPorts = effect->getPorts(true).size();
            auto numOutputPorts = effect->getPorts(false).size();

            if (numOutputPorts != 0) {
                // Add an output port to parent
                parentEffect->addPort(ConnectionGraph::getInstance()->getDefaultBus(), false);
            }
            if (numInputPorts != 0) {
                // Add an input port to parent
                parentEffect->addPort(ConnectionGraph::getInstance()->getDefaultBus(), true);
            }
        }

        // Auto-connect children
        auto childrenEffects = parentEffect->getFullConnectionEffectsInside();
        if (childrenEffects.size() == 0) {
            // Create inside connection
            //todo
        } else {
            // Add to inside connection
            //todo
        }

        // Adjust size to fit all children
        refitChildren(parentEffect);

        // Auto-place based on connections
        //autoPlace(effect);
    }

    movingOp = false;
}

void EffectPositioner::effectCreated(Effect *effect) {

}

void EffectPositioner::effectDeleted(Effect *effect) {

}


ConnectionLine *EffectPositioner::getConnectionToPort(ConnectionPort_old *port) const {
    auto effect = dynamic_cast<Effect*>(port->getParentComponent());
    for (auto connection : effect->getConnectionsToThis()) {
        if (connection->getInPort() == port) {
            return connection;
        } else if (connection->getOutPort() == port) {
            return connection;
        }
    }
    return nullptr;
}

void EffectPositioner::swapEffects(Effect *effectDragged, Effect *effectToMove) {
    std::cout << "Dragged effect pos: " << effectDragged->getX() + effectDragged->getWidth() / 2 << newLine;
    std::cout << "Swapped effect pos: " << effectToMove->getX() + effectToMove->getHeight() / 2 << newLine;

    //fixme only for single ports!
    auto inPortsToMove = effectToMove->getPorts(true);
    auto outPortsToMove = effectToMove->getPorts(false);
    jassert(inPortsToMove.size() == 1);
    jassert(outPortsToMove.size() == 1);

    auto inPortsDragged = effectDragged->getPorts(true);
    auto outPortsDragged = effectDragged->getPorts(false);
    jassert(inPortsDragged.size() == 1);
    jassert(outPortsDragged.size() == 1);

    if (inPortsToMove.getFirst()->getOtherPort() == outPortsDragged.getFirst()) {
        // Switch over towards the right
        auto middleConnection = getConnectionToPort(inPortsToMove.getFirst());
        auto startConnection = getConnectionToPort(inPortsDragged.getFirst());
        mergeConnection(startConnection, middleConnection);

        jassert(effectDragged->getConnectionsToThis(true, ConnectionLine::audio).size() == 0);

        auto endConnection = getConnectionToPort(outPortsToMove.getFirst());
        insertEffect(effectDragged, endConnection);

        jassert(effectDragged->getConnectionsToThis(true, ConnectionLine::audio).size() == 1);
        jassert(effectDragged->getConnectionsToThis(false, ConnectionLine::audio).size() == 1);

        makeRoomForEffect(effectDragged);

    } else if (outPortsToMove.getFirst()->getOtherPort() == inPortsDragged.getFirst()) {
        // Switch over towards the left
        auto middleConnection = getConnectionToPort(outPortsToMove.getFirst());
        auto endConnection = getConnectionToPort(outPortsDragged.getFirst());
        mergeConnection(middleConnection, endConnection);

        jassert(effectDragged->getConnectionsToThis(false, ConnectionLine::audio).size() == 0);

        auto startConnection = getConnectionToPort(inPortsToMove.getFirst());
        insertEffect(effectDragged, startConnection);

        jassert(effectDragged->getConnectionsToThis(true, ConnectionLine::audio).size() == 1);
        jassert(effectDragged->getConnectionsToThis(false, ConnectionLine::audio).size() == 1);

        makeRoomForEffect(effectDragged);

    } else {
        //jassertfalse;
    }
}

EffectPositioner::EffectPositioner() {
    instance = this;
}

EffectPositioner *EffectPositioner::getInstance() {
    jassert(instance != nullptr);
    return instance;
}

/**
 *
 * @param leftEffect
 * @param rightEffect
 * @return distance needing to be added in order to make the effects fit next to each other
 */
int EffectPositioner::getFittedDistance(const Effect *leftEffect, const Effect *rightEffect) const {
    return rightEffect->getX() - minDistanceBetweenEffects - leftEffect->getRight();
}

void EffectPositioner::moveEffect(Effect* effect, int distance, bool rightWard) {
    if (rightWard) {
        effect->setTopLeftPosition(effect->getX() + distance, effect->getY());
    } else {
        effect->setTopLeftPosition(effect->getX() - distance, effect->getY());
    }
}


void EffectPositioner::refitChildren(Effect *parentEffect) {
    // Set size based on children

    auto totalWidth = 200;
    auto totalHeight = 200;

    for (auto c : parentEffect->getChildren()) {
        if (auto effect = dynamic_cast<Effect*>(c)) {
            totalWidth += effect->getWidth() + minDistanceBetweenEffects;
            totalHeight = jmax(totalHeight, effect->getBottom()); //todo adjust for parameters, parameter mods
        }
    }

    parentEffect->setSize(totalWidth, totalHeight);
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

        auto newPort = parentToExtendThrough->addPort(ConnectionGraph::getInstance()->getDefaultBus(), isInput);
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

        auto newPort = parentToExtendThrough->addPort(ConnectionGraph::getInstance()->getDefaultBus(), isInput);
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

void EffectPositioner::removeEffectConnections(Effect *effect) {
    auto inputConnections = effect->getConnectionsToThis(true, ConnectionLine::audio);
    auto outputConnections = effect->getConnectionsToThis(false, ConnectionLine::audio);

    for (auto i = 0; i < jmax(inputConnections.size(), outputConnections.size()); i++) {
        if (i < inputConnections.size() && i < outputConnections.size()) {
            // Merge connection
            mergeConnection(inputConnections[i], outputConnections[i]);
        } else if (i < inputConnections.size()) {
            // Remove from input connections
            inputConnections[i]->getParentComponent()->removeChildComponent(inputConnections[i]);
        } else if (i < outputConnections.size()) {
            // Remove from output connections
            outputConnections[i]->getParentComponent()->removeChildComponent(outputConnections[i]);
        }
    }

}

int EffectPositioner::getDistanceFromLineExtended(Line<int> line, Point<int> point) const
{
    auto angle = line.getAngle();
    auto extensionDistance = 100;

    //todo check all this shit!
    auto extensionDistX = sinf(angle) * extensionDistance;
    auto extensionDistY = cosf(angle) * extensionDistance;
    auto extension = Point<int>(extensionDistX, extensionDistY);

    auto extendedLine = Line<int>(line.getStart() - extension, line.getEnd() + extension);

    auto d1 = extendedLine.getStart().getDistanceFrom(point);
    auto d2 = extendedLine.getEnd().getDistanceFrom(point);
    return d1 + d2 - extendedLine.getLength();
}

void EffectPositioner::insertEffect(Effect *effect, ConnectionLine *line) {
    //todo deal with this later
    jassert(effect->getConnectionsToThis(true, ConnectionLine::audio).isEmpty());
    jassert(effect->getConnectionsToThis(false, ConnectionLine::audio).isEmpty());

    auto inPorts = effect->getPorts(true);
    auto outPorts = effect->getPorts(false);

    if (inPorts.size() > 0 && outPorts.size() > 0) {
        //todo deal with this as well
        //jassert(! inPorts.getFirst()->isConnected() && ! outPorts.getFirst()->isConnected());

        auto endPort = line->getInPort();
        line->unsetPort(line->getInPort());
        line->setPort(inPorts.getFirst());

        auto newline = new ConnectionLine();
        effect->getParentComponent()->addAndMakeVisible(newline);
        newline->setPort(outPorts.getFirst());
        newline->setPort(endPort);
    }

}

Point<int> EffectPositioner::getEffectCenter(Effect *effect) const
{
    return scene->getLocalPoint(effect, Point<int>(effect->getWidth(), effect->getHeight()) / 2);
}

void EffectPositioner::setPositionerRunning(bool runState) {
    jassert(instance != nullptr);
    instance->positionerRunning = runState;
}

void EffectPositioner::setScene(EffectBase *scene) {
    this->scene = scene;
}

/**
 * Auto-positions the whole line of connections connected up to and after the given effect.
 * @param effect: target effect, should only have maximum one input port and one output port for now..
 */
void EffectPositioner::autoPlace(Effect *effect) {
    if (effect->getPorts(true).size() == 1 && effect->getPorts(false).size() == 1) {
        auto connectedLeft = effect->getFullConnectionEffects(effect->getPorts(true).getFirst());
        auto connectedRight = effect->getFullConnectionEffects(effect->getPorts(false).getFirst());

        auto fullConnection = connectedLeft;
        fullConnection.add(effect);
        fullConnection.addArray(connectedRight);

        int posX;
        int posY;

        if (dynamic_cast<Effect*>(effect->getParentComponent())) {
            posX = effect->getParentComponent()->getX() + minDistanceBetweenEffects;
            posY = effect->getParentComponent()->getHeight() / 2;
        } else {
            posX = connectedLeft.getLast()->getRight() + minDistanceBetweenEffects;
            posY = connectedLeft.getLast()->getY() + connectedLeft.getLast()->getHeight() / 2; //todo linear height between start and end
        }

        for (auto i = 1; i < fullConnection.size(); i++) {
            auto effectToPosition = fullConnection[i];
            effectToPosition->setCentrePosition(posX + effectToPosition->getWidth()
                    , posY); //todo ratio-height
            posX += effectToPosition->getWidth() + minDistanceBetweenEffects;
        }
        //int totalConnectionLength = jmax(rightPos - leftPos, necessaryConnectionLength);
    }

    if (auto parentEffect = dynamic_cast<Effect*>(effect->getParentComponent())) {
        autoPlace(parentEffect);
    }
}

/**
 * Different type of auto-place function, this one is incremental, making room for each
 * effect if needed going down the connected effects one by one.
 * @param effect
 */
void EffectPositioner::makeRoomForEffect(Effect *effect) {
    if (effect->getPorts(true).size() == 1 && effect->getPorts(false).size() == 1) {
        auto connectedLeft = effect->getFullConnectionEffects(effect->getPorts(true).getFirst());
        auto connectedRight = effect->getFullConnectionEffects(effect->getPorts(false).getFirst());

        // Make room towards the left
        auto previousEffect = effect;
        for (auto e : connectedLeft) {
            fitEffects(e, previousEffect, false);
            previousEffect = e;
        }

        // Make room towards the right
        previousEffect = effect;
        for (auto e : connectedRight) {
            fitEffects(previousEffect, e, true);
            previousEffect = e;
        }

    }
}

/**
 * Basically just moves one of the two effects so that there's room in between.
 * @param leftEffect effect on the left
 * @param rightEffect effect on the right
 * @param expandRightwards: if true, then rightEffect is moved. else leftEffect is moved.
 */
void EffectPositioner::fitEffects(Effect *leftEffect, Effect *rightEffect, bool expandRightwards) {
    if (rightEffect->getX() - minDistanceBetweenEffects < leftEffect->getRight()) {
        int distanceToExpand = -(rightEffect->getX() - minDistanceBetweenEffects - leftEffect->getRight());

        moveEffect(expandRightwards ? rightEffect : leftEffect
                , distanceToExpand, expandRightwards);
    }

}



