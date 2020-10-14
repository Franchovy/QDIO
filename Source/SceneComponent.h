/*
  ==============================================================================

    SceneComponent.h
    Created: 14 Oct 2020 10:34:39am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SceneComponent : public Component, public ReferenceCountedObject
{
public:
    SceneComponent();
    void paint(Graphics &g) override;

    void setHoverable(bool isHoverable);
    void setSelectable(bool isSelectable);
    void setDraggable(bool isDraggable);
    void setDragExitable(bool isDragExitable);
    void setExitDraggable(bool isExitDraggable);

    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void addChildComponent(SceneComponent child);

private:
    const int SELECT_MAX_DISTANCE = 5;

    static ReferenceCountedArray<SceneComponent> selectedComponents;

    /// Array to maintain a referenceCount for children
    ReferenceCountedArray<SceneComponent> children;

    bool isDraggable = false;
    bool isDragExitable = false;
    bool isExitDraggable = false;
    ComponentDragger dragger;

    bool selected = false;
    bool selectable = false;
    bool hovered = false;
    bool hoverable = false;
};