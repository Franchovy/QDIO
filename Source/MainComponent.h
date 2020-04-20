/*
  ==============================================================================

    MainComponent.h
    Created: 13 Apr 2020 4:18:45pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EffectScene.h"
#include "IDs"
#include "Settings.h"

/**
 * This is the class that contains the main / static stuff. The EffectScene is part of the effect tree,
 * as the tree-top, and the MainComponent/Viewport acts as its manager.
 */
class MainComponent : public Viewport, private Timer
{
public:
    //
    explicit MainComponent();

    ~MainComponent() override;


private:

    void move(int deltaX, int deltaY);
    void timerCallback() override;

    EffectScene main;
    AudioDeviceManager& deviceManager;

    ImageButton settingsButton;
    SettingsComponent settingsMenu;

    ValueTree effectTree;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
