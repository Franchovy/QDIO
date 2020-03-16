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


/**
 * Main component and shit
 */
class MainComponent   :
        public ValueTree::Listener, public Timer, public Component,  public LassoSource<Component*>, public ChangeListener {
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

    //==============================================================================
    // AudioSource overrides
    /*void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override;*/

    //void addEffect(GUIEffect::Ptr effectPtr);

    void timerCallback() override { updateGraph(); }

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

    Point<int> menuPos;
    ValueTree getTreeFromComponent(Component* g, String name);

    // GUI Helper tools
    LineComponent dragLine;
    LassoComponent<Component*> lasso;

    void findLassoItemsInArea (Array <Component*>& results, const Rectangle<int>& area) override;
    Array<Component*> componentsToSelect;
    SelectedItemSet<Component*> selected;
    SelectedItemSet<Component*>& getLassoSelection() override;

    // GUI Helper tools for effect navigation
    Array<GUIEffect*> effectsAt(Point<int> point){
        Array<GUIEffect*> list;
        for (int i = 0; i < effectsTree.getNumChildren(); i++){
            if (effectsTree.getChild(i).getType() == ID_EFFECT_TREE){
                // Check at location
                auto e_gui = dynamic_cast<GUIEffect*>(effectsTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());
                if (e_gui->contains(point))
                    list.add(e_gui);
            }
        }
        return list;
    }

    // Listener callback on SelectedItemSet "selected" change broadcast.
    void changeListenerCallback (ChangeBroadcaster *source) override;

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
                    auto node = processorGraph->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
                    auto e = createEffect(event, node);
                    addEffect(event, e);
                }));
        m.addItem("Output Effect", std::function<void()>(
                [=]{
                    auto node = processorGraph->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));
                    auto e = createEffect(event, node);
                    addEffect(event, e);
                }));
        return m;
    }

    //GUIEffect*

    /**
     * Adds existing effect as child to the effect under the mouse
     * @param event for which the location will determine what effect to add to.
     * @param childEffect effect to add
     */
    void addEffect(const MouseEvent& event, EffectVT::Ptr childEffect, bool addAnyways = true) {
        if (auto effectWrapper = dynamic_cast<GUIWrapper*>(event.originalComponent)){
            if (auto effectGUI = dynamic_cast<GUIEffect*>(effectWrapper->getChild())){
                auto parentEffect = effectGUI->EVT;
                parentEffect->addEffect(childEffect);
            }
        } else if (addAnyways) {
            effectsTree.appendChild(childEffect->getTree(), nullptr);
        }
    }

    /**
     * Empty
     */
    EffectVT::Ptr createEffect(const MouseEvent &event, AudioProcessorGraph::Node::Ptr node = nullptr) {
        if (node != nullptr){
            // Individual effect from processor
            return new EffectVT(event, node->nodeID);
        }
        if (selected.getNumSelected() == 0){
            // Empty effect
            return new EffectVT(event);
        } else if (selected.getNumSelected() > 0){
            // Create Effect with selected Effects inside
            Array<const EffectVT*> effectVTArray;
            for (auto eGui : selected.getItemArray()){
                // Selected includes GUIWrapper....
                effectVTArray.add(dynamic_cast<GUIEffect*>(eGui)->EVT);
            }
            return new EffectVT(event, effectVTArray);
        }

    }

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
    Identifier ID_EFFECT_VT = "Effect"; // This is the class that has been defined - where ID_EFFECT_TREE point to
    //Identifier ID_EFFECT_GUI = "GUI";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


