/*
  ==============================================================================

    EffectSceneComponent.cpp
    Created: 15 Oct 2020 11:18:51am
    Author:  maxime

  ==============================================================================
*/

#include "EffectSceneComponent.h"

EffectSceneComponent::EffectSceneComponent() {
    setColour(backgroundID, Colours::grey);

    //=======================================================================
    // Test area:

    childComponent.setHoverable(true);
    childComponent.setSelectable(true);
    childComponent.setDraggable(true);
    childComponent.setExitDraggable(true);
    addSceneComponent(&childComponent);
    childComponent.setBounds(getHeight() / 2, getWidth() / 2, 200, 200);

    parentComponent.setDragExitable(true);
    parentComponent.setHoverable(true);
    addSceneComponent(&parentComponent);
    parentComponent.setBounds(1000, 500, 500, 500);

    effect.setBounds(500, 500, 200, 200);
    addSceneComponent(&effect);
    effect.setNumPorts(1, 2);
}
