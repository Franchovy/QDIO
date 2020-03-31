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
class ComponentSelection : public SelectedItemSet<Component*>
{
public:
    ComponentSelection() : SelectedItemSet<Component*>() {};

    void itemSelected(Component *type) override;
    void itemDeselected(Component *type) override;
};



/**
 * Main component and shit
 */
class MainComponent   :
        public ValueTree::Listener, public Timer, public Component,
        public LassoSource<Component*>, /*public ChangeListener, */private ReferenceCountedObject {
public:
    bool keyPressed(const KeyPress &key) override;

    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    // Component overrides
    void paint (Graphics&) override;
    void resized() override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    //==============================================================================
    // AudioSource overrides
    /*void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override;*/

    //void addEffect(GUIEffect::Ptr effectPtr);

    void timerCallback() override {
        // do something interesting here
    }

    // Create Connection - called from LineComponent (lineDrag)
    void createConnection(std::unique_ptr<ConnectionLine> newConnection);

    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;


private:
    //==============================================================================
    // Effect tree shit
    ValueTree effectsTree;

    void valueTreeChildAdded (ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                               int indexFromWhichChildWasRemoved) override;

    CustomMenuItems mainMenu;
    Point<int> menuPos;

    // GUI Helper tools
    LineComponent dragLine;
    LassoComponent<Component*> lasso;
    bool intersectMode = true;

    void findLassoItemsInArea (Array <Component*>& results, const Rectangle<int>& area) override;
    Array<Component*> componentsToSelect;
    ComponentSelection selected;
    SelectedItemSet<Component*>& getLassoSelection() override;

    void setHoverComponent(Component* c) {
        if (auto e = dynamic_cast<GUIEffect*>(hoverComponent)) {
            e->hoverMode = false;
            e->repaint();
        } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent)) {
            p->hoverMode = false;
            p->repaint();
        }

        hoverComponent = c;

        if (auto e = dynamic_cast<GUIEffect*>(hoverComponent)) {
            e->hoverMode = true;
            e->repaint();
        } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent)) {
            p->hoverMode = true;
            p->repaint();
        }
    };

    Component* hoverComponent = nullptr;

    Component* effectToMoveTo(Component* componentToIgnore, Point<int> point, ValueTree effectTree);
    ConnectionPort* portToConnectTo(MouseEvent& event, ValueTree effectTree);

    //==============================================================================
    String KEYNAME_DEVICE_SETTINGS = "audioDeviceState";

    //
    PopupMenu getEffectSelectMenu(const MouseEvent &event){
        PopupMenu m;
        m.addItem("Empty Effect", std::function<void()>(
                [=]{
                    auto e = createEffect(event);
                    addEffect(event, e);
                }));
        m.addItem("Input Effect", std::function<void()>(
                [=]{
                    auto node = processorGraph->addNode(std::make_unique<InputDeviceEffect>());
                    auto e = createEffect(event, node);
                    addEffect(event, e);
                }));
        m.addItem("Output Effect", std::function<void()>(
                [=]{
                    auto node = processorGraph->addNode(std::make_unique<OutputDeviceEffect>());
                    auto e = createEffect(event, node);
                    addEffect(event, e);
                }));
        m.addItem("Delay Effect", std::function<void()>(
                [=](){
                    auto node = processorGraph->addNode(std::make_unique<DelayEffect>());
                    node->getProcessor()->setPlayConfigDetails(
                                processorGraph->getMainBusNumInputChannels(),
                                processorGraph->getMainBusNumOutputChannels(),
                                processorGraph->getSampleRate(),
                                processorGraph->getBlockSize()
                            );
                    auto e = createEffect(event, node);
                    addEffect(event, e);
                }
                ));
        m.addItem("Distortion Effect", std::function<void()>(
                [=]{
                    auto node = processorGraph->addNode(std::make_unique<DistortionEffect>());
                    node->getProcessor()->setPlayConfigDetails(
                            processorGraph->getMainBusNumInputChannels(),
                            processorGraph->getMainBusNumOutputChannels(),
                            processorGraph->getSampleRate(),
                            processorGraph->getBlockSize()
                    );
                    auto e = createEffect(event, node);
                    addEffect(event, e);
                }
                ));
        return m;
    }

    /**
     * Adds existing effect as child to the effect under the mouse
     * @param event for which the location will determine what effect to add to.
     * @param childEffect effect to add
     */
    void addEffect(const MouseEvent& event, EffectVT::Ptr childEffect, bool addToMain = true) {
        auto parentTree = childEffect->getTree().getParent();
        parentTree.removeChild(childEffect->getTree(), nullptr);

        ValueTree newParent;
        if (auto newGUIEffect = dynamic_cast<GUIEffect*>(event.eventComponent)){
            newParent = newGUIEffect->EVT->getTree();
        } else
            newParent = effectsTree;

        newParent.appendChild(childEffect->getTree(), nullptr);
    }

    EffectVT::Ptr createEffect(const MouseEvent &event, AudioProcessorGraph::Node::Ptr node = nullptr);

    //==============================================================================
    // Connections
    ReferenceCountedArray<ConnectionLine> connections;

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

    void addAudioConnection(ConnectionLine* connectionLine);


        // MIDI
    /*
    ComboBox midiInputList;
    Label midiInputListLabel;
    int lastInputIndex = 0;

    void setMidiInput (int index);
*/


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


