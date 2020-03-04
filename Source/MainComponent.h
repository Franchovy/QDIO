/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "Effector.h"


ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();


//template<class ComponentType>
class GUIWrapper : public Component
{
public:
    GUIWrapper() {
        setSize(800,800);
        size.setXY(getWidth(), getHeight());
        outline.setBounds(0,0,getWidth(),getHeight());

        addChildComponent(resizer);
        resizer.setAlwaysOnTop(true);

        closeButton.setButtonText("Close");
        addChildComponent(closeButton);
        closeButton.onClick = [=]{
            setVisible(false);
        };

        addChildComponent(title);
    }

    void paint (Graphics& g) override {
        g.drawRect(outline);
    }
    void resized() override {

        title.setBounds(size.x/2 - 30, 10, 80, 30);
        closeButton.setBounds(size.x - 80, 10, 70, 30);
        outline.setBounds(0,0,size.x,size.y);
        for (auto c : childComponents)
            c->setBounds(30,30,size.x-30,size.y-30);
        size.setXY(getWidth(),getHeight());
    }

    void mouseDown(const MouseEvent& event) override {
        dragger.startDraggingComponent(this, event);
        if (event.mods.isRightButtonDown())
            getParentComponent()->mouseDown(event);
        Component::mouseDown(event);
    }
    void mouseDrag(const MouseEvent& event) override {
        dragger.dragComponent(this, event, nullptr);
    }

    PopupMenu& getMenu(){
        return menu;
    }

    void addToMenu(String item){
        menu.addItem(item);
    }

    void setTitle(String name){
        title.setText(name,dontSendNotification);
    }

    void setVisible(bool shouldBeVisible) override {
        closeButton.setVisible(shouldBeVisible);
        resizer.setVisible(shouldBeVisible);
        title.setVisible(shouldBeVisible);
        for (auto c : childComponents){
            c->setVisible(shouldBeVisible);
        }
        Component::setVisible(shouldBeVisible);
    }

    void childrenChanged() override {
        childComponents.clear();
        for (auto c : getChildren()){
            if (c != &resizer && c != &closeButton && c != &title)
                childComponents.add(c);
        }
        Component::childrenChanged();
        resized();
    }

    ~GUIWrapper() override {
        childComponents.clear();
    }

    void parentSizeChanged() override {
        // Keep size
        setSize(size.x, size.y);
    }

    TextButton closeButton;

private:
    Point<int> size; // unchanging size
    Label title;
    Rectangle<float> outline;
    ComponentDragger dragger;
    PopupMenu menu;
    Resizer resizer;
    Array<Component*> childComponents;
};


/**
 * Main component and shit
 */
class MainComponent   : public Component,
        public ValueTree::Listener, public Timer
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
    // AudioSource overrides
    /*void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override;*/

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
    String KEYNAME_DEVICE_SETTINGS = "audioDeviceState";

    //==============================================================================
    // Audio shit
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;

    AudioDeviceManager deviceManager;
    AudioProcessorPlayer player;
    std::unique_ptr<AudioProcessorGraph> processorGraph;

    GUIWrapper deviceSelectorComponent;
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
