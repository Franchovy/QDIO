/*
  ==============================================================================

    EffectGui.h
    Created: 5 Apr 2020 11:20:04am
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>

#pragma once



class GuiObject : public ReferenceCountedObject, public Component
{
public:
    using Ptr = ReferenceCountedObjectPtr<GuiObject>;

    GuiObject() = default;
    ~GuiObject() override {
        //resetReferenceCount();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuiObject)
};

class ComponentSelection;

class SelectHoverObject : public GuiObject
{
public:
    SelectHoverObject();
    ~SelectHoverObject();

    using Ptr = ReferenceCountedObjectPtr<SelectHoverObject>;

    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void setHoverable(bool isHoverable);
    static SelectHoverObject* getHoverObject();
    static void setHoverObject(SelectHoverObject::Ptr item);
    static void resetHoverObject();

    static void addSelectObject(const SelectHoverObject::Ptr& item);
    static void removeSelectObject(const SelectHoverObject::Ptr& item);
    void deselectAll();
    ReferenceCountedArray<SelectHoverObject> getSelected();

    static void close();

    void mouseDrag(const MouseEvent &event) override;

    /// "Shall we look inside?"
    virtual bool canDragInto(const SelectHoverObject* other) const = 0;
    /// "Should this trigger hover?"
    virtual bool canDragHover(const SelectHoverObject* other) const = 0;

    void startDragHoverDetect();
    void endDragHoverDetect();
    static SelectHoverObject* getDragIntoObject();

protected:
    static ReferenceCountedArray<SelectHoverObject> componentsToSelect;
    static ComponentSelection selected;

    bool hoverMode = false;
    bool selectMode = false;

    bool hoverable = true;

private:
    static SelectHoverObject::Ptr hoverComponent;
    static bool manualHover;

    static SelectHoverObject* draggedComponent;
    static SelectHoverObject* dragIntoComponent;

    static SelectHoverObject* findDragHovered(SelectHoverObject* objectToCheck);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelectHoverObject)
};


/**
 * SelectedItemSet for Component* class, with
 * itemSelected/itemDeselected overrides. That is all.
 */
class ComponentSelection : public SelectedItemSet<SelectHoverObject::Ptr>
{
public:
    ComponentSelection() = default;
    ~ComponentSelection() = default;

    void clear() {
        SelectedItemSet<SelectHoverObject::Ptr>::deselectAll();
    }



    void itemSelected(SelectHoverObject::Ptr type) override;
    void itemDeselected(SelectHoverObject::Ptr object) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentSelection)
};


class Resizer : public GuiObject
{
public:
    Resizer() :
            strokeType(1.f)
    {
        setSize(30,30);
        box.setSize(getWidth(), getHeight());
    }

    void paint(Graphics& g) override {
        //g.drawRect(box);
    }

    void mouseDown(const MouseEvent &event) override{
        startPos = Point<float>(getX() + event.getMouseDownX(), getY() + event.getMouseDownY());
        dragger.startDraggingComponent(this, event);
        setMouseCursor(MouseCursor::DraggingHandCursor);
        getParentComponent()->mouseDown(event);
        //Component::mouseDown(event);
    }
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override{
        setMouseCursor(MouseCursor::ParentCursor);
        Component::mouseUp(event);
    }

    void parentHierarchyChanged() override {
        setPosition();
        Component::parentHierarchyChanged();
    }

    ~Resizer() override
    {
    }

    void setPosition() {
        setTopLeftPosition(getParentComponent()->getWidth() - getWidth(), getParentComponent()->getHeight() - getHeight());
    }

    void parentSizeChanged() override {
        setPosition();
        Component::parentSizeChanged();
    }

    void mouseEnter(const MouseEvent &event) override {
        setMouseCursor(MouseCursor::TopLeftCornerResizeCursor);
        Component::mouseEnter(event);
    }

    void mouseExit(const MouseEvent &event) override {
        setMouseCursor(getParentComponent()->getMouseCursor());
        Component::mouseExit(event);
    }

private:
    Point<float> startPos;
    Rectangle<float> box;
    ComponentDragger dragger;

    MouseCursor prevMouseCursor;

    PathStrokeType strokeType;
    float dashLengths[2] = {1.f, 1.f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Resizer)
};
