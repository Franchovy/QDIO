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
SelectHoverObject* SelectHoverObject::dragIntoComponent = nullptr;


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
    std::cout << "Mouse Enter " << event.eventComponent->getName() << newLine;
    if (hoverable && ! manualHover) {
        setHoverObject(this);
    }
    Component::mouseEnter(event);
}

void SelectHoverObject::mouseExit(const MouseEvent &event) {
    std::cout << "Mouse Exit " << event.eventComponent->getName() << newLine;
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
    std::cout << "mouse down: " << event.eventComponent->getName() << newLine;
    Component::mouseDown(event);
}

void SelectHoverObject::mouseDrag(const MouseEvent &event) {
    if (auto obj = dynamic_cast<SelectHoverObject*>(event.eventComponent)) {
        auto parent = dynamic_cast<SelectHoverObject*>(obj->getParentComponent());
        jassert(parent != nullptr);
        if (parent != nullptr) {
            dragIntoComponent = findDragHovered(parent);
        }
    }

    Component::mouseDrag(event);
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

SelectHoverObject* SelectHoverObject::getDragIntoObject() {
    return dragIntoComponent;
}

SelectHoverObject* SelectHoverObject::findDragHovered(SelectHoverObject* objectToCheck) {
    if (objectToCheck->contains(objectToCheck->getMouseXYRelative())) {
        // Check if mouse is hovering over children
        for (auto c : objectToCheck->getChildren()) {
            if (auto child = dynamic_cast<SelectHoverObject*>(c)) {
                // Return child if it contains mouse
                if (child->contains(child->getMouseXYRelative())) {
                    // Checks
                    if (child == draggedComponent) {
                        continue;
                    }

                    if (objectToCheck->canDragInto(child)) {
                        return findDragHovered(child);
                    }/* else if (objectToCheck->canDragHover(child)) {
                        return child;
                    }*/ else {
                        continue;
                    }
                }
            }
        }
        // Return nullptr on no change
        return nullptr;
    } else {
        // Return parent if mouse is no longer in this object
        auto parent = dynamic_cast<SelectHoverObject*>(objectToCheck->getParentComponent());

        jassert(parent != nullptr);
        return parent;
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

