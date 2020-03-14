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

    void valueTreeChildAdded (ValueTree &parentTree, ValueTree &childWhichHasBeenAdded);
    UndoManager undoManager;
    Point<int> menuPos;
    ValueTree getTreeFromComponent(Component* g, String name);

    // GUI Helper class and callback for connections
    LineComponent dragLine;
    LassoComponent<Component*> lasso;

    void findLassoItemsInArea (Array <Component*>& results, const Rectangle<int>& area) override;
    Array<Component*> componentsToSelect;
    SelectedItemSet<Component*> selected;
    SelectedItemSet<Component*>& getLassoSelection() override;

    // Listener callback on SelectedItemSet "selected" change broadcast.
    void changeListenerCallback (ChangeBroadcaster *source) override;

    //==============================================================================
    String KEYNAME_DEVICE_SETTINGS = "audioDeviceState";

    //
    PopupMenu getEffectSelectMenu(){
        PopupMenu m;
        m.addItem("Empty Effect", std::function<void()>(
                [=]{
                    createEffect();
                }));
        m.addItem("Input Effect", std::function<void()>(
                [=]{
                    auto node = processorGraph->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
                    createEffect(node);
                }));
        m.addItem("Output Effect", std::function<void()>(
                [=]{
                    auto node = processorGraph->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));
                    createEffect(node);}));
        return m;
    }

    /**
     * Empty
     */
    EffectVT::Ptr createEffect(AudioProcessorGraph::Node::Ptr node = nullptr){
        if (node != nullptr){
            // Individual effect from processor
            return new EffectVT(node->nodeID);
        }
        if (selected.getNumSelected() == 0){
            // Empty effect
            return new EffectVT();
        } else if (selected.getNumSelected() > 0){
            // Create Effect with selected Effects inside
            Array<const EffectVT*> effectVTArray;
            for (auto eGui : selected.getItemArray()){
                effectVTArray.add(dynamic_cast<GUIEffect*>(eGui)->EVT);
            }
            return new EffectVT(effectVTArray);
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
    Identifier ID_EFFECT_VT = "Effect"; // This is the class that has been defined - where ID_EFFECT_TREE point to
    //Identifier ID_EFFECT_GUI = "GUI";

    //void createEffect(GUIEffect::Ptr parent = nullptr);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


