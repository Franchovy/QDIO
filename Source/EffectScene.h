/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/



#pragma once

#include <JuceHeader.h>

#include "Effector.h"
#include "IOEffects.h"
#include "BaseEffects.h"

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

const String KEYNAME_DEVICE_SETTINGS = "audioDeviceState";

/**
 * SelectedItemSet for Component* class, with
 * itemSelected/itemDeselected overrides. That is all.
 */
class ComponentSelection : public SelectedItemSet<GuiObject::Ptr>
{
public:
    ComponentSelection() = default;

    void clear() {
        SelectedItemSet<GuiObject::Ptr>::deselectAll();
    }

    void itemSelected(GuiObject::Ptr type) override;
    void itemDeselected(GuiObject::Ptr type) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentSelection)
};


/**
 *
 */
class EffectScene   :
        public ValueTree::Listener, public EffectTreeBase,
        public LassoSource<GuiObject::Ptr>, public ComponentListener
{
public:

    //==============================================================================
    EffectScene();
    ~EffectScene();

    //==============================================================================
    // Component overrides
    void paint (Graphics&) override;
    void resized() override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    bool keyPressed(const KeyPress &key) override;

    void deleteEffect(GuiEffect* e);
    //==============================================================================

private:
    // Effect tree shit
    ValueTree effectsTree;

    void valueTreeChildAdded (ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                               int indexFromWhichChildWasRemoved) override;
    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;

    //==============================================================================
    // Audio shit
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;


    //std::unique_ptr<AudioDeviceSelectorComponent> deviceSelector;
    //GUIWrapper deviceSelectorComponent;

    //==============================================================================

    std::unique_ptr<CustomMenuItems> mainMenu;

    // GUI Helper tools
    LineComponent dragLine;
    LassoComponent<GuiObject::Ptr> lasso;
    bool intersectMode = true;

    // TODO LassoComponent change to a different pointer type/
    void findLassoItemsInArea (Array <GuiObject::Ptr>& results, const Rectangle<int>& area) override;
    ReferenceCountedArray<GuiObject> componentsToSelect;
    ComponentSelection selected;
    SelectedItemSet<GuiObject::Ptr>& getLassoSelection() override;

    void setHoverComponent(GuiObject::Ptr c);
    GuiObject::Ptr hoverComponent = nullptr;

    Component* effectToMoveTo(Component* componentToIgnore, Point<int> point, ValueTree effectTree);
    static ConnectionPort::Ptr portToConnectTo(MouseEvent& event, const ValueTree& effectTree);

    //==============================================================================

    EffectVT::Ptr createEffect(const MouseEvent &event, const AudioProcessorGraph::Node::Ptr& node = nullptr);
    void addEffect(const MouseEvent& event, EffectVT::Ptr childEffect, bool addToMain = true);
    PopupMenu getEffectSelectMenu(const MouseEvent &event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectScene)
};


/**
 * This is the class that contains the main / static stuff. The EffectScene is part of the effect tree,
 * as the tree-top, and the MainComponent/Viewport acts as its manager.
 */
class MainComponent : public Viewport, private Timer
{
public:
    MainComponent() {
        auto savedState = getAppProperties().getUserSettings()->getXmlValue (KEYNAME_DEVICE_SETTINGS);

        main.initialiseAudio(
                std::make_unique<AudioProcessorGraph>(),
                std::make_unique<AudioDeviceManager>(),
                std::make_unique<AudioProcessorPlayer>(),
                std::move(savedState)
        );

        setViewedComponent(&main);
        addAndMakeVisible(main);
        setBounds(0,0, 1920, 1080);

        startTimer(3);
    }

    ~MainComponent(){
        main.close();
    }

private:

    void move(int deltaX, int deltaY) {
        if (deltaX != 0  || deltaY != 0) {
            setViewPosition(getViewPositionX() - deltaX, getViewPositionY() - deltaY);
        }
    }

    void timerCallback() override {
        // scrolling
        auto pos = getMouseXYRelative();
        if (! getBounds().contains(pos))
            return;

        int deltaX = 0;
        int deltaY = 0;
        if (pos.x < 150)
            deltaX = (150 - pos.x) / 2;
        else if (pos.x > getWidth() - 150)
            deltaX = (getWidth() - 150 - pos.x) / 2;

        if (pos.y < 150)
            deltaY = (150 - pos.y) / 2;
        else if (pos.y > getHeight() - 150)
            deltaY = (getHeight() - 150 - pos.y) / 2;

        move(deltaX, deltaY);

        startTimer(3);
    }

    EffectScene main;
    UndoManager undoManager;

};
