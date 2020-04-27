/*
  ==============================================================================

    EffectGui.cpp
    Created: 5 Apr 2020 11:20:04am
    Author:  maxime

  ==============================================================================
*/

#include "EffectGui.h"

SelectHoverObject::Ptr SelectHoverObject::hoverComponent = nullptr;
ComponentSelection SelectHoverObject::selected;
ReferenceCountedArray<SelectHoverObject> SelectHoverObject::componentsToSelect;


// ==============================================================================
// Resizer methods

void Resizer::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);

    getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                  startPos.y + event.getDistanceFromDragStartY());
    Component::mouseDrag(event);
}

void SelectHoverObject::setHoverComponent(SelectHoverObject::Ptr item) {
    resetHoverObject();

    if (item != nullptr) {
        item->hoverMode = true;
        hoverComponent = item;
    }
}

SelectHoverObject::SelectHoverObject() {
    setRepaintsOnMouseActivity(true);

    if (getName() != "MainWindow") {
        componentsToSelect.add(this);
    }
}

SelectHoverObject::~SelectHoverObject() {
    if (getReferenceCount() > 0) {
        componentsToSelect.removeObject(this);

        if (hoverComponent == this) {
            hoverComponent = nullptr;
        }

        selected.deselect(this);

        if (getReferenceCount() > 0) {
            std::cout << "shit gonna hit the fan!!" << newLine;
        }
    }
}

void SelectHoverObject::resetHoverObject() {
    if (hoverComponent != nullptr) {
        hoverComponent->hoverMode = false;
        hoverComponent->repaint();
        hoverComponent = nullptr;
    }
}

void SelectHoverObject::setHoverable(bool isHoverable) {
    hoverable = isHoverable;
}

void SelectHoverObject::mouseEnter(const MouseEvent &event) {
    if (hoverable) {
        setHoverComponent(this);
    } else {
        Component::mouseEnter(event);
    }
}

void SelectHoverObject::mouseExit(const MouseEvent &event) {
    if (hoverable) {
        resetHoverObject();
    } else {
        Component::mouseEnter(event);
    }
}

void SelectHoverObject::setSelectMode(bool newSelectMode) {
    if (newSelectMode) {
        addSelectObject(this);
    } else {
        removeSelectObject(this);
    }
}

void SelectHoverObject::addSelectObject(const SelectHoverObject::Ptr& item) {
    item->selectMode = true;

    item->repaint();
    selected.addToSelection(item);
}

void SelectHoverObject::removeSelectObject(const SelectHoverObject::Ptr& item) {
    item->selectMode = false;
    item->repaint();
    selected.deselect(item);
}

void SelectHoverObject::close() {
    componentsToSelect.clear();
    selected.clear();
    hoverComponent = nullptr;
}

void SelectHoverObject::mouseDown(const MouseEvent &event) {
    Component::mouseDown(event);
}

void SelectHoverObject::mouseUp(const MouseEvent &event) {
    if (event.getDistanceFromDragStart() < 10) {
        addSelectObject(this);
    }
    Component::mouseUp(event);
}
