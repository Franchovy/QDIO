/*
  ==============================================================================

    Scene.cpp
    Created: 15 Oct 2020 11:18:51am
    Author:  maxime

  ==============================================================================
*/

#include "Scene.h"

Scene::Scene() {
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

    effect2.setBounds(900, 200, 200, 200);
    addSceneComponent(&effect2);
    effect2.setNumPorts(2, 2);

    addConnection(this, effect.getPorts()[0], effect2.getPorts()[0]);

}

void Scene::mouseDown(const MouseEvent &event) {
    SceneComponent::mouseDown(event);
}

void Scene::mouseDrag(const MouseEvent &event) {
    SceneComponent::mouseDrag(event);
}

void Scene::mouseUp(const MouseEvent &event) {
    SceneComponent::mouseUp(event);
}

bool Scene::canDragHover(const SceneComponent &other, bool isRightClickDrag) const {
    return dynamic_cast<const EffectConnectable*>(&other) != nullptr;
}

bool Scene::canDragInto(const SceneComponent &other, bool isRightClickDrag) const {
    return dynamic_cast<const EffectConnectable*>(&other) != nullptr;
}
