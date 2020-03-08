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

    //
    PopupMenu getEffectSelectMenu(){
        PopupMenu m;
        m.addItem("Empty Effect", std::function<void()>(
                [=]{
                    auto e = new EffectVT(processorGraph.get(), "Empty Effect");
                    createEffect(e->getNode(), "Effect");
                }));
        m.addItem("Input Effect", std::function<void()>(
                [=]{
                    createEffect<AudioGraphIOProcessor>("Input Device", AudioGraphIOProcessor::audioInputNode);}));
        m.addItem("Output Effect", std::function<void()>(
                [=]{
                    createEffect<AudioGraphIOProcessor>("Output Device", AudioGraphIOProcessor::audioOutputNode);}));
        return m;
    }

    template <class Processor>
    void createEffect(String name, AudioProcessorGraph::AudioGraphIOProcessor::IODeviceType deviceType){
        if (std::is_same<Processor, AudioProcessorGraph::AudioGraphIOProcessor>()){
            auto node = processorGraph->addNode(
                    std::make_unique<AudioGraphIOProcessor>(deviceType));
            node->getProcessor()->getName();
            createEffect(node, name);
        } else {
            std::cout << "Invalid AudioGraphIOProcessor Type";
        }
    }

    template<class Processor>
    void createEffect(String name, AudioProcessor* processor){
         //else if (std::is_same<Processor, EffectProcessor) //TODO Base effect?
    }

    void createEffect(AudioProcessorGraph::Node::Ptr node, String name){
        EffectVT::Ptr e = new EffectVT(node->nodeID, processorGraph.get(), name);
        effectsTree.appendChild(e->getTree(),nullptr); //TODO fix mem management

        std::cout << "Test: " << newLine;
        e->getProcessor().getBus
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
    Identifier ID_EFFECT_GUI = "GUI";

    //void createEffect(GUIEffect::Ptr parent = nullptr);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
