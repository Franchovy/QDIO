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

/**
 * SelectedItemSet for Component* class, with
 * itemSelected/itemDeselected overrides. That is all.
 */
class ComponentSelection : public SelectedItemSet<GuiObject::Ptr>
{
public:
    ComponentSelection() = default;
    ~ComponentSelection() = default;

    void clear() {
        SelectedItemSet<GuiObject::Ptr>::deselectAll();
    }

    void itemSelected(GuiObject::Ptr type) override;
    void itemDeselected(GuiObject::Ptr type) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentSelection)
};


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

    static void setHoverComponent(SelectHoverObject::Ptr item);
    static void resetHoverObject();

    static void addSelectObject(const SelectHoverObject::Ptr& item);
    static void removeSelectObject(const SelectHoverObject::Ptr& item);

    void setHoverable(bool isHoverable);
    void setSelectMode(bool newSelectMode);

protected:
    static SelectHoverObject::Ptr hoverComponent;
    static ReferenceCountedArray<SelectHoverObject> componentsToSelect;
    static ComponentSelection selected;

    static void close();

    bool hoverable = true;

    bool hoverMode = false;
    bool selectMode = false;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelectHoverObject)
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
        setCentrePosition(getParentComponent()->getWidth(), getParentComponent()->getHeight());
        Component::parentHierarchyChanged();
    }

    ~Resizer() override
    {
    }

    void parentSizeChanged() override {
        setCentrePosition(getParentWidth(), getParentHeight());
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
