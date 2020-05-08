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

/**
 *
 */
class EffectScene   :
        public EffectTreeBase
{
public:

    //==============================================================================
    EffectScene();
    ~EffectScene() override;

    //==============================================================================
    // Component overrides
    void paint (Graphics&) override;
    void resized() override;

    /*void mouseWheelMove(const MouseEvent &event, const MouseWheelDetails &wheel) override;*/
    
    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;

    void mouseUp(const MouseEvent &event) override;

    bool keyPressed(const KeyPress &key) override;

    void storeState();

    //void updateChannels();

    void handleCommandMessage(int commandId) override;

    AudioDeviceManager& getDeviceManager() { return deviceManager; }
    AudioProcessorGraph& getAudioGraph() { return audioGraph; }
    AudioProcessorPlayer& getAudioPlayer() { return processorPlayer; }

    struct IDs {
        static const Identifier DeviceManager;
    };

    Rectangle<int> view;

private:
    // EffectTreeBase static stuff
    EffectTree updater;

    AudioDeviceManager deviceManager;
    AudioProcessorPlayer processorPlayer;
    AudioProcessorGraph audioGraph;

    //==============================================================================
    // Audio shit
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;

    Image bg;
    Image bgTile;
    Image logo;
    Point<int> tileDelta;


    PopupMenu mainMenu;
    PopupMenu createEffectMenu;
    PopupMenu::Item createGroupEffectItem;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectScene)
};

