/*
  ==============================================================================

    MainComponent.h
    Created: 13 Apr 2020 4:18:45pm
    Author:  maxime

  ==============================================================================
*/
//#define JUCE_ENABLE_REPAINT_DEBUGGING 1

#pragma once

#include <JuceHeader.h>
#include "EffectScene.h"
#include "IDs"
#include "Settings.h"
#include "EffectSelectMenu.h"

/**
 * This is the class that contains the main / static stuff. The EffectScene is part of the effect tree,
 * as the tree-top, and the MainComponent/Viewport acts as its manager.
 */
class MainComponent : public Viewport, private Timer, public ChangeListener
{
public:
    //
    explicit MainComponent();

    ~MainComponent() override;

    void handleCommandMessage(int commandId) override;

    void resized() override;
    void changeListenerCallback(ChangeBroadcaster *source) override;
    /*void mouseWheelMove(const MouseEvent &event, const MouseWheelDetails &wheel) override;*/

    void updateEffectSelectMenu();

private:
    int deltaX;
    int deltaY;

    void move(int deltaX, int deltaY);
    void timerCallback() override;

    void populateEffectMenu(PopupMenu& menu);

    EffectScene main;
    AudioProcessorGraph& audioGraph;
    AudioDeviceManager& deviceManager;
    AudioProcessorPlayer& audioPlayer;

    ImageButton settingsButton;
    SettingsComponent settingsMenu;

    EffectSelectMenu effectSelectMenu;

    ValueTree effectTree;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
