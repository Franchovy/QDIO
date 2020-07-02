/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/



#pragma once

#include <JuceHeader.h>
#include <BinaryData.h>

#include "Effect.h"
#include "EffectTree.h"
#include "IDs"
#include "EffectLoader.h"
#include "MenuItem.h"


/**
 *
 */
class EffectScene : public EffectTreeBase, public MenuItem, public Timer, public ChangeListener
{
public:

    //==============================================================================
    EffectScene();
    ~EffectScene() override;

    void timerCallback() override;

    void changeListenerCallback(ChangeBroadcaster *source) override;

    //==============================================================================
    // Component overrides
    void paint (Graphics&) override;
    void resized() override;

    /*void mouseWheelMove(const MouseEvent &event, const MouseWheelDetails &wheel) override;*/

    void menuCreateEffect(ValueTree effect);
    void loadNewTemplate(String newTemplate);

    int callSaveTemplateDialog(String& name, bool dontSaveButton);
    int callSaveEffectDialog(String &name);

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    bool keyPressed(const KeyPress &key) override;

    void saveTemplate();
    void importTemplate();
    void exportTemplate();
    void saveEffect();
    void importEffect();
    void exportEffect();

    void refreshBuffer();

    void storeState();

    void handleCommandMessage(int commandId) override;

    AudioDeviceManager& getDeviceManager() { return deviceManager; }
    AudioProcessorGraph& getAudioGraph() { return audioGraph; }
    AudioProcessorPlayer& getAudioPlayer() { return processorPlayer; }

    static EffectScene* getScene() { return instance; }

    struct IDs {
        static const Identifier DeviceManager;
    };

    Rectangle<int> view;

    // Menu stuff
    //Array<PopupMenu::Item> setupCreateEffectMenu();
    StringArray getProcessorNames();
    void createProcessor(int processorID);

    bool canDragInto(const SelectHoverObject *other, bool isRightClickDrag = false) const override;
    bool canDragHover(const SelectHoverObject *other, bool isRightClickDrag = false) const override;

private:
    //todo replace usage of this instance with EffectTreeBase::effectScene
    static EffectScene* instance;

    bool loadInitialCase = false;
    bool dontLoad = false;

    // EffectTreeBase static stuff
    EffectTree tree;

    AudioDeviceManager deviceManager;
    AudioProcessorPlayer processorPlayer;
    AudioProcessorGraph audioGraph;

    ConnectionGraph connectionGraph;

    //==============================================================================
    // Audio shit
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;

    Image bg;
    Image bgTile;
    Image logo;
    Point<int> tileDelta;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectScene)
};

