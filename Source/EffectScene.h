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

/*class EffectsScene : public Component, */



/**
 *
 */
class EffectScene   :
        public ValueTree::Listener, public Component,
        public LassoSource<GuiObject::Ptr>, private ReferenceCountedObject {
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

    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    bool keyPressed(const KeyPress &key) override;

    void deleteEffect(GUIEffect* e);
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

    AudioDeviceManager deviceManager;
    AudioProcessorPlayer player;

    AudioDeviceSelectorComponent deviceSelector;

    std::unique_ptr<AudioProcessorGraph> processorGraph;

    GUIWrapper deviceSelectorComponent;

    int numInputChannels = 2;
    int numOutputChannels = 2;

    void addAudioConnection(ConnectionLine* connectionLine);
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
    static ConnectionPort* portToConnectTo(MouseEvent& event, const ValueTree& effectTree);

    //==============================================================================
    String KEYNAME_DEVICE_SETTINGS = "audioDeviceState";

    EffectVT::Ptr createEffect(const MouseEvent &event, const AudioProcessorGraph::Node::Ptr& node = nullptr);
    void addEffect(const MouseEvent& event, EffectVT::Ptr childEffect, bool addToMain = true);
    PopupMenu getEffectSelectMenu(const MouseEvent &event);


    //==============================================================================
    // Connections
    ReferenceCountedArray<ConnectionLine> connections;

    


        // MIDI
    /*
    ComboBox midiInputList;
    Label midiInputListLabel;
    int lastInputIndex = 0;

    void setMidiInput (int index);
*/


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectScene)

};


/**
 * This is the class that contains the main / static stuff. The EffectScene is part of the effect tree,
 * as the tree-top, and the MainComponent/Viewport acts as its manager.
 */
class MainComponent : public Viewport
{
public:
    MainComponent() {
        setViewedComponent(&main);
    }

private:

    EffectScene main;
};
