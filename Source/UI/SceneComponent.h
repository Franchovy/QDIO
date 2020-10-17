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
    ~SceneComponent();

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

    void addSceneComponent(SceneComponent* child);
    void removeSceneComponent(SceneComponent* child);

    Array<SceneComponent*> getSelectedComponents();
    template<class Type> Array<Type*> getSelectedComponentsOfType();
    void deselectAll();
    void select();
    void toggleSelect();

    virtual bool canDragHover(const SceneComponent& other, bool isRightClickDrag) const {return false;}
    virtual bool canDragInto(const SceneComponent& other, bool isRightClickDrag) const {return false;}
    virtual void targetHoverMouseUp(const MouseEvent &event, SceneComponent* targetComponent);

    enum ColourIDs {
        backgroundID = 0,
        hoverID,
        selectID,
        outlineID
    };

    //=========================================================
    // Frame and structure

    struct Geometry {
        int cornerSize = 5;
        int boundarySize = 20;
        int lineThicness = 3;
    } geometry;

protected:
    bool hovered = false;
    bool selected = false;

private:
    const int SELECT_MAX_DISTANCE = 5;

    static ReferenceCountedArray<SceneComponent> selectedComponents;

    /// Array to maintain a referenceCount for children
    ReferenceCountedArray<SceneComponent> children;

    bool isDraggable = false;
    bool isDragExitable = false;
    bool isExitDraggable = false;
    ComponentDragger dragger;

    bool selectable = false;
    bool hoverable = false;

    SceneComponent* targetHoverComponent = nullptr;
};