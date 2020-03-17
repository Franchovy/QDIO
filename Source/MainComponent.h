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
    struct TreeTop : public ReferenceCountedObject
    {
        // TODO merge this with EffectVT somehow

        TreeTop(String name) : effectTree(name) { }

        // Convenience functions
/*        EffectVT::Ptr getParent(){ return dynamic_cast<EffectVT*>(
                    effectTree.getParent().getProperty(ID_EFFECT_VT).getObject())->ptr(); }*/
        EffectVT::Ptr getChild(int index){ return dynamic_cast<EffectVT*>(
                    effectTree.getChild(index).getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
        int getNumChildren() { return effectTree.getNumChildren(); }
        void appendChild(EffectVT::Ptr child) { effectTree.appendChild(child->getTree(), nullptr); }
        const ValueTree &getTree() const { return effectTree; }
        void addListener(ValueTree::Listener *listener){ effectTree.addListener(listener); }
        const ValueTree &getDragLineTree() const { return dragLineTree; }
        void setDragLineTree(ValueTree dragLineTree) { TreeTop::dragLineTree = dragLineTree; }
        EffectVT* getParent(){ return nullptr; }


    private:
        ValueTree effectTree;
        ValueTree dragLineTree;
    };
    TreeTop effectsTree;

    void valueTreeChildAdded (ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                               int indexFromWhichChildWasRemoved) override;

    Point<int> menuPos;

    // GUI Helper tools
    LineComponent dragLine;
    LassoComponent<Component*> lasso;
    bool intersectMode = true;

    void findLassoItemsInArea (Array <Component*>& results, const Rectangle<int>& area) override;
    Array<Component*> componentsToSelect;
    SelectedItemSet<Component*> selected;
    SelectedItemSet<Component*>& getLassoSelection() override;

    // GUI Helper tools for effect navigation
    Array<GUIEffect*> effectsAt(Point<int> point){
        Array<GUIEffect*> list;
        for (int i = 0; i < effectsTree.getNumChildren(); i++){
            // Check at location
            auto e_gui = effectsTree.getChild(i)->getGUIEffect();
            if (e_gui->getBoundsInParent().contains(point)) {
                //TODO Add recursive call for a match
                std::cout << "Effect at point: " << e_gui->getName();
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

    /**
     * Adds existing effect as child to the effect under the mouse
     * @param event for which the location will determine what effect to add to.
     * @param childEffect effect to add
     */
    void addEffect(const MouseEvent& event, EffectVT::Ptr childEffect, bool addAnyways = true) {
        std::cout << "Add effect to be removed" << newLine;
        if (auto effectGUI = dynamic_cast<GUIEffect*>(event.eventComponent)){
            auto parentEffect = effectGUI->EVT;
            parentEffect->addEffect(childEffect);
        } else if (addAnyways) {
            effectsTree.appendChild(childEffect);
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
                effectVTArray.add(dynamic_cast<GUIEffect*>(eGui)->EVT);
            }
            return new EffectVT(event, effectVTArray);
        }
    }

    /**
     * Generalised operation to run on an effect drag release.
     * UNDOABLE ACTION
     * @param event - Event to get information from - event.originalComponent should
     * be = effect (or its corresponding GUIWrapper), while event.eventComponent is the
     * "object" of the operation - the new parent effect, which
     * @param effect should be operated on.
     */
     void moveEffect(const MouseEvent &event, EffectVT::Ptr effect) {
         Component* targetEffect;
         Component* parentEffect;
         bool targetIsMainWindow;
         bool parentIsMainWindow;

         if (event.eventComponent == this) {
             targetEffect = this;
             targetIsMainWindow = true;
         } else {
             targetEffect = dynamic_cast<GUIEffect*>(event.eventComponent);
             targetIsMainWindow = false;
         }

         if (effect->getTree().getParent() == effectsTree.getTree()) {
             parentEffect = this;
             parentIsMainWindow = true;
         } else {
             parentEffect = effect->getParent()->getGUIEffect();
             parentIsMainWindow = false;
         }

         // Nothing to do on a targetIsMainWindow && parentIsMainWindow case.
         if (targetIsMainWindow && parentIsMainWindow){
             return;
         } else if (targetIsMainWindow && !parentIsMainWindow){
             std::cout << "Target is Mainwindow" << newLine;
         } else if (!targetIsMainWindow && parentIsMainWindow){
             std::cout << "Parent is Mainwindow" << newLine;
             dynamic_cast<GUIEffect*>(targetEffect)->EVT->addEffect(effect);
         } else if (!targetIsMainWindow && !parentIsMainWindow) {
             std::cout << "Neither are Mainwindow" << newLine;
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
    static void updateGraph();

    // MIDI
    /*
    ComboBox midiInputList;
    Label midiInputListLabel;
    int lastInputIndex = 0;

    void setMidiInput (int index);
*/


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


