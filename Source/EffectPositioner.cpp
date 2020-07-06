/*
  ==============================================================================

    EffectPositioner.cpp
    Created: 6 Jul 2020 10:52:30am
    Author:  maxime

  ==============================================================================
*/

#include "EffectPositioner.h"

void EffectPositioner::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {


    ComponentListener::componentMovedOrResized(component, wasMoved, wasResized);
}
