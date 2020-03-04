/*
  ==============================================================================

    Effector.h
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"


class EffectProcessor : public AudioProcessor
{
public:

    //===============================================================================
    // Function overrides - all the default crap
    const String getName() const override { return String(); }
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    AudioProcessorEditor *createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return String(); }
    void changeProgramName(int index, const String &newName) override {}
    void getStateInformation(juce::MemoryBlock &destData) override {}
    void setStateInformation(const void *data, int sizeInBytes) override {}

};

class Resizer : public Component
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
        Component::mouseDown(event);
    }
    void mouseDrag(const MouseEvent &event) override{
        dragger.dragComponent(this, event, nullptr);

        getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                      startPos.y + event.getDistanceFromDragStartY());
        Component::mouseDrag(event);
    }
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

};

/**
    GUIEffect Component
    GUI Representation of Effects / Container for plugins
    ReferenceCountedObject for usage as part of ValueTree system
*/
class GUIEffect  : public Component, public ReferenceCountedObject
{
public:
    GUIEffect ();
    ~GUIEffect() override;

    using Ptr = ReferenceCountedObjectPtr<GUIEffect>;

    void paint (Graphics& g) override;
    void resized() override;

    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;


private:
    Rectangle<float> outline;
    Resizer resizer;

    ComponentDragger dragger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIEffect)
};

/**
   EffectVT (ValueTree) is the manager of everything in an Effect.

   This includes GUI, AudioProcessor, and the Effect's ValueTree itself.

   It's a little confusing because the ValueTree owned by the object goes on to refer to this object - but the
   destructor should take care of that.
 */


class EffectVT : public ReferenceCountedObject
{
public:
    EffectVT() : effectTree("effectTree")
    {
        // Set this object as a property of the owned valuetree....
        effectTree.setProperty("effectVT", this, nullptr);
        // Set GUI object as property
        effectTree.setProperty("GUI", &gui, nullptr);
    }
    ~EffectVT()
    {
        effectTree.removeProperty("effectVT", nullptr);
    }

    using Ptr = ReferenceCountedObjectPtr<EffectVT>;

    ValueTree& getTree() {return effectTree;}

private:
    ValueTree effectTree;
    GUIEffect gui;

};
