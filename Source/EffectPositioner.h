/*
  ==============================================================================

    EffectPositioner.h
    Created: 6 Jul 2020 10:52:30am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"

class EffectPositioner : public ComponentListener
{
public:
    EffectPositioner();

    static EffectPositioner* getInstance();

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;

    void detachFromConnection(Effect* effect);
    void attachToConnection(Effect* effect, ConnectionLine* connection);

private:
    static EffectPositioner* instance;

    bool movingOp = false;

    // Magic numbers
    int minDistanceBetweenEffects = 40;
};