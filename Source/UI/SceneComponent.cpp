/*
  ==============================================================================

    SceneComponent.cpp
    Created: 14 Oct 2020 10:34:39am
    Author:  maxime

  ==============================================================================
*/

#include "SceneComponent.h"

ReferenceCountedArray<SceneComponent> SceneComponent::selectedComponents;


SceneComponent::SceneComponent() : Component() {
    setColour(backgroundID, Colours::lightgrey);
    setColour(hoverID, Colours::blue);
    setColour(selectID, Colours::ghostwhite);
    setColour(outlineID, Colours::darkslategrey);
}

void SceneComponent::setHoverable(bool isHoverable) {
    setRepaintsOnMouseActivity(isHoverable);
    hoverable = isHoverable;
}

void SceneComponent::setSelectable(bool isSelectable) {
    selectable = isSelectable;
    if (! isSelectable) {
        selectedComponents.removeObject(this);
    }
}

void SceneComponent::setDraggable(bool isDraggable) {
    this->isDraggable = isDraggable;
}

/**
 * isDragExitable implies child SceneComponent child objects can be dragged into and
 * out of it.
 * @param isDragExitable
 */
void SceneComponent::setDragExitable(bool isDragExitable) {
    this->isDragExitable = isDragExitable;
}

/**
 * isExitDraggable implies this component can be dragged in and out of SceneComponent parents
 * that have isDragExitable = true.
 * @param isExitDraggable
 */
void SceneComponent::setExitDraggable(bool isExitDraggable) {
    this->isExitDraggable = isExitDraggable;
}

void SceneComponent::paint(Graphics &g) {
    Rectangle<int> outline = getLocalBounds();
    Rectangle<int> frame = outline.expanded(-geometry.boundarySize);

    DropShadow shadow(Colours::black.withAlpha(0.5f), 15, Point<int>());
    shadow.drawForRectangle(g, frame);

    if (selected && selectable) {
        g.setColour(findColour(selectID));
        g.drawRect(outline);
    }
    if (hovered && hoverable) {
        g.setColour(findColour(hoverID));
        g.drawRect(outline);
    }
    // Fill inside frame
    g.setColour(findColour(backgroundID));
    g.fillRoundedRectangle(frame.toFloat(), geometry.cornerSize);
    // Draw Outline
    g.setColour(findColour(outlineID));
    g.drawRoundedRectangle(frame.toFloat(), geometry.cornerSize, geometry.lineThicness);


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

void SceneComponent::targetHoverMouseUp(const MouseEvent &event, SceneComponent *targetComponent) {
    auto newPosition = getPosition() - targetComponent->getPosition();
    targetComponent->addAndMakeVisible(this);
    setTopLeftPosition(newPosition);
}

Array<SceneComponent *> SceneComponent::getSelectedComponents() {
    Array<SceneComponent*> array;
    for (auto s : selectedComponents) {
        array.add(s);
    }
    return array;
}

template<class Type>
Array<Type *> SceneComponent::getSelectedComponentsOfType() {
    Array<Type*> array;
    for (auto s : selectedComponents) {
        if (dynamic_cast<Type*>(s)) {
            array.add(s);
        }
    }
    return array;
}

void SceneComponent::deselectAll() {

}

void SceneComponent::select() {

}

void SceneComponent::toggleSelect() {

}

void SceneComponent::addSceneComponent(SceneComponent *child) {
    children.add(child);
    Component::addAndMakeVisible(child);
}

void SceneComponent::removeSceneComponent(SceneComponent *child) {
    children.removeObject(child);
    Component::removeChildComponent(child);
}

void SceneComponent::mouseDown(const MouseEvent &event) {
    if (isDraggable) {
        toFront(false);
        dragger.startDraggingComponent(this, event);
    }
    Component::mouseDown(event);
}

void SceneComponent::mouseDrag(const MouseEvent &event) {
    if (isDraggable) {
        // Drag this component
        dragger.dragComponent(this, event, nullptr);

        // Detect object being hovered
        for (auto c : this->getParentComponent()->getChildren()) {
            if (c == this) continue;
            if (c->contains(c->getLocalPoint(event.eventComponent, event.getPosition()))) {
                if (auto target = dynamic_cast<SceneComponent*>(c)) {
                    targetHoverComponent = target;
                    targetHoverComponent->hovered = true;
                    targetHoverComponent->repaint();

                    // Visual react if component is dragInto-able
                    if (target->canDragInto(*this, false)) {
                        target->holdSizeOnHover(100);
                    }

                }
            }
        }
        // Deselect any target component if not hovered
        if (targetHoverComponent != nullptr && ! targetHoverComponent->contains(
                targetHoverComponent->getLocalPoint(event.eventComponent, event.getPosition()))) {
            targetHoverComponent->resetHoverSize();
            targetHoverComponent->hovered = false;
            targetHoverComponent->repaint();
            targetHoverComponent = nullptr;
        }
    }
    Component::mouseDrag(event);
}

void SceneComponent::mouseUp(const MouseEvent &event) {
    //  Operation on target hover component
    if (targetHoverComponent != nullptr) {
        if (targetHoverComponent->canDragInto(*this, false)) {
            targetHoverMouseUp(event, targetHoverComponent);
        }


        targetHoverComponent->hovered = false;
        targetHoverComponent->repaint();
        targetHoverComponent = nullptr;
    }

    // Select this object
    if (event.getDistanceFromDragStart() < SELECT_MAX_DISTANCE && selectable) {
        selected = ! selected;
        if(selected) {
            selectedComponents.add(this);
        } else {
            selectedComponents.removeObject(this);
        }
    }
    Component::mouseUp(event);
}

SceneComponent::~SceneComponent() {
    if (auto parent = dynamic_cast<SceneComponent*>(getParentComponent())) {
        parent->removeSceneComponent(this);
    }
}

void SceneComponent::holdSizeOnHover(int deltaSize) {
    auto centerPosition = Point<int>(getX() + getWidth() / 2, getY() + getHeight() / 2);
    setSize(getWidth() + deltaSize - holdingHoverSize, getHeight() + deltaSize - holdingHoverSize);
    holdingHoverSize = deltaSize;
    setCentrePosition(centerPosition);

}

void SceneComponent::resetHoverSize() {
    auto centerPosition = Point<int>(getX() + getWidth() / 2, getY() + getHeight() / 2);
    setSize(getWidth() - holdingHoverSize,
            getHeight() - holdingHoverSize);
    setCentrePosition(centerPosition);
    holdingHoverSize = 0;
}





