/*
  ==============================================================================

    SceneComponent.cpp
    Created: 14 Oct 2020 10:34:39am
    Author:  maxime

  ==============================================================================
*/

#include "SceneComponent.h"

ReferenceCountedArray<SceneComponent> SceneComponent::selectedComponents;


SceneComponent::SceneComponent() {

}

void SceneComponent::paint(Graphics &g) {
    Rectangle<int> outline = getBounds();
    if (selected) {
        g.setColour(Colours::darkblue);
        g.drawRect(outline);
    }
    if (hovered) {
        g.setColour(Colours::pink);
        g.drawRect(outline);
    }
    Component::paint(g);
}

void SceneComponent::mouseEnter(const MouseEvent &event) {
    hovered = true;
    Component::mouseEnter(event);
}

void SceneComponent::mouseExit(const MouseEvent &event) {
    hovered = false;
    Component::mouseExit(event);
}

void SceneComponent::mouseDown(const MouseEvent &event) {
    Component::mouseDown(event);
}

void SceneComponent::mouseDrag(const MouseEvent &event) {
    Component::mouseDrag(event);
}

void SceneComponent::mouseUp(const MouseEvent &event) {
    if (event.getDistanceFromDragStart() < SELECT_MAX_DISTANCE) {
        // Select this object
        selectedComponents.add(this);
        selected = ! selected;
    }
    Component::mouseUp(event);
}

