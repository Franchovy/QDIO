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
