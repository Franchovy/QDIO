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

bool SelectHoverObject::manualHover = false;
SelectHoverObject* SelectHoverObject::draggedComponent = nullptr;


// ==============================================================================
// Resizer methods

void Resizer::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);

    getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                  startPos.y + event.getDistanceFromDragStartY());
    Component::mouseDrag(event);
}

void SelectHoverObject::setHoverObject(SelectHoverObject::Ptr item) {
    resetHoverObject();

    if (item != nullptr) {
        item->hoverMode = true;
        std::cout << "hover mode on" << newLine;
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
    /*if (getReferenceCount() > 0) {
        componentsToSelect.removeObject(this);

        if (hoverComponent == this) {
            hoverComponent = nullptr;
        }

        selected.deselect(this);

        if (getReferenceCount() > 0) {
            std::cout << "shit gonna hit the fan!!" << newLine;
        }
    }*/
}

void SelectHoverObject::resetHoverObject() {
    if (hoverComponent != nullptr) {
        hoverComponent->hoverMode = false;
        hoverComponent = nullptr;
    }
}

void SelectHoverObject::setHoverable(bool isHoverable) {
    hoverable = isHoverable;
}

void SelectHoverObject::mouseEnter(const MouseEvent &event) {
    if (hoverable && ! manualHover) {
        setHoverObject(this);
    }
    Component::mouseEnter(event);
}

void SelectHoverObject::mouseExit(const MouseEvent &event) {
    if (hoverable && ! manualHover) {
        resetHoverObject();
    }
    Component::mouseEnter(event);
}

void SelectHoverObject::addSelectObject(const SelectHoverObject::Ptr& item) {
    item->selectMode = true;

    selected.addToSelection(item);
}

void SelectHoverObject::removeSelectObject(const SelectHoverObject::Ptr& item) {
    item->selectMode = false;
    //item->repaint();
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

        if (selectMode) {
            removeSelectObject(this);
        } else {
            addSelectObject(this);
        }
    }
    Component::mouseUp(event);
}

void SelectHoverObject::deselectAll() {
    selected.deselectAll();
    repaint();
}

ReferenceCountedArray<SelectHoverObject> SelectHoverObject::getSelected() {
    ReferenceCountedArray<SelectHoverObject> itemArray;
    for (const auto& item : selected.getItemArray()) {
        itemArray.add(item);
    }
    return itemArray;
}

SelectHoverObject *SelectHoverObject::getHoverObject() {
    return hoverComponent.get();
}

void SelectHoverObject::startDragHoverDetect() {
    draggedComponent = this;
    manualHover = true;
}

void SelectHoverObject::endDragHoverDetect() {
    draggedComponent = nullptr;
    manualHover = false;
}

void SelectHoverObject::findDragHovered(SelectHoverObject* currentHoveredComponent) {
    auto mousePos = currentHoveredComponent->getMouseXYRelative();

    for (auto childComponent : currentHoveredComponent->getChildren()) {
        if (childComponent->contains(childComponent->getLocalPoint(currentHoveredComponent, mousePos))) {
            // Checks before call
            auto childObject = dynamic_cast<SelectHoverObject*>(childComponent);
            if (childObject == nullptr) {
                continue;
            }
            if (childObject == draggedComponent) {
                continue;
            }
            setHoverObject(childObject);
            childObject->repaint();
            findDragHovered(childObject);
        }
    }
}

void SelectHoverObject::mouseDrag(const MouseEvent &event) {
    if (! hoverComponent->contains(event.getPosition())) {
        findDragHovered(hoverComponent.get());
    }
    Component::mouseDrag(event);
}

SelectHoverObject* SelectHoverObject::getDragIntoObject() const {
    if (hoverComponent == draggedComponent) {
        return nullptr;
    } else {
        return hoverComponent.get();
    }
}

void ComponentSelection::itemSelected(SelectHoverObject::Ptr object){
    SelectHoverObject::addSelectObject(object);
    SelectedItemSet::itemSelected(object);
}

void ComponentSelection::itemDeselected(SelectHoverObject::Ptr object) {
    SelectHoverObject::removeSelectObject(object);
    SelectedItemSet::itemDeselected(object);
}

