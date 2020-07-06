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

    if (wasResized) {
        for (auto c : effect->getConnectionsToThis()) {
            if (c->type == ConnectionLine::audio) {
                // get full end connection
                for (auto port : effect->getPorts()) {
                    if (port->isConnected()) {

                        auto connectedEffects = effect->getFullConnectionEffects(port); //fixme returns first effect twice.
                        if (connectedEffects.size() == 0)
                            continue;

                        auto leftEffect = port->isInput ? connectedEffects.getFirst() : effect;
                        auto rightEffect = port->isInput ? effect : connectedEffects.getFirst();

                        if (leftEffect->getRight() > rightEffect->getX() - minDistanceBetweenEffects) {
                            int distanceToMove = (port->isInput ? -1 : 1)
                                    * (leftEffect->getRight() - (rightEffect->getX() - minDistanceBetweenEffects)); //fixme this returns rather large distance
                            for (auto e : connectedEffects) {
                                movingOp = true;
                                // Scooch the effects
                                e->setTopLeftPosition(e->getX() + distanceToMove, e->getY());
                                movingOp = false;
                            }
                        }
                    }
                }

                // move full end by size change.
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
