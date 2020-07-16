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

                        // Get Connections and Effects from port onwards
                        auto connectedEffects = effect->getFullConnectionEffects(port); //fixme returns first effect twice.
                        if (connectedEffects.size() == 0)
                            continue;

                        // Set the left and right effects to check position for
                        auto leftEffect = port->isInput ? connectedEffects.getFirst() : effect;
                        auto rightEffect = port->isInput ? effect : connectedEffects.getFirst();

                        int distanceMin = rightEffect->getX() - minDistanceBetweenEffects - leftEffect->getRight();

                        if (distanceMin < 0) {
                            for (auto e : connectedEffects) {
                                movingOp = true;
                                // Scooch the effects
                                if (port->isInput) e->setTopLeftPosition(e->getX() + distanceMin, e->getY());
                                if (! port->isInput) e->setTopLeftPosition(e->getX() - distanceMin, e->getY());
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
