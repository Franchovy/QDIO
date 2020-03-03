/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "Effector.h"

class GUIWrapper : public Component
{
public:
    GUIWrapper(){

    }

    void paint (Graphics&) override {
    }
    void resized() override {
    }

    void mouseDown(const MouseEvent& event) override {
        dragger.startDraggingComponent(this, event);
    }
    void mouseDrag(const MouseEvent& event) override {
        dragger.dragComponent(this, event, nullptr);
    }

    PopupMenu& getMenu(){
        return menu;
    }
private:
    ComponentDragger dragger;
    PopupMenu menu;
};


/**
 * Main component and shit
 */
class MainComponent   : public AudioAppComponent, public ValueTree::Listener, public Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    // Component overrides
    void paint (Graphics&) override;
    void resized() override;
    void mouseDown(const MouseEvent &event) override;

    //==============================================================================
    // AudioAppComponent overrides
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override;

    //void addEffect(GUIEffect::Ptr effectPtr);

    void timerCallback() override { updateGraph(); }

private:
    //==============================================================================
    // Effect tree shit
    ValueTree effectsTree;
    void valueTreeChildAdded (ValueTree &parentTree, ValueTree &childWhichHasBeenAdded);

    UndoManager undoManager;

    Point<int> menuPos;

    ValueTree getTreeFromComponent(Component* g, String name);

    //==============================================================================
    // Audio shit
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;

    AudioDeviceManager deviceManager;
    AudioProcessorPlayer player;
    std::unique_ptr<AudioProcessorGraph> processorGraph;

    GroupComponent deviceSelectorGroup;
    AudioDeviceSelectorComponent deviceSelector;
    ToggleButton hideDeviceSelector;

    int numInputChannels = 2;
    int numOutputChannels = 2;

    Node::Ptr audioInputNode;
    Node::Ptr audioOutputNode;
    Node::Ptr midiInputNode;
    Node::Ptr midiOutputNode;

    MidiBuffer emptyMidiMessageBuffer;

    void initialiseGraph();
    void connectAudioNodes();
    void connectMidiNodes();
    void updateGraph();

    // MIDI
    /*
    ComboBox midiInputList;
    Label midiInputListLabel;
    int lastInputIndex = 0;

    void setMidiInput (int index);
*/

    // StringRefs - move these to Includer file
    Identifier ID_TREE_TOP = "Treetop";
    Identifier ID_EFFECT_TREE = "effectTree";
    Identifier ID_EFFECT_GUI = "GUI";

    //void createEffect(GUIEffect::Ptr parent = nullptr);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
