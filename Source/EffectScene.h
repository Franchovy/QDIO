/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/



#pragma once

#include <JuceHeader.h>
#include <BinaryData.h>

#include "Effect.h"

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

const String KEYNAME_DEVICE_SETTINGS = "audioDeviceState";
const String KEYNAME_LOADED_EFFECTS = "loadedEffects";


/**
 *
 */
class EffectScene   :
        public EffectTreeBase
{
public:

    //==============================================================================
    EffectScene();
    ~EffectScene();

    //==============================================================================
    // Component overrides
    void paint (Graphics&) override;
    void resized() override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    bool keyPressed(const KeyPress &key) override;

    void deleteEffect(Effect* e);

    //==============================================================================

private:
    //==============================================================================
    // Audio shit
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;

    //std::unique_ptr<AudioDeviceSelectorComponent> deviceSelector;
    //GUIWrapper deviceSelectorComponent;
    Image bg;


    //==============================================================================

    PopupMenu mainMenu;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectScene)
};


/**
 * This is the class that contains the main / static stuff. The EffectScene is part of the effect tree,
 * as the tree-top, and the MainComponent/Viewport acts as its manager.
 */
class MainComponent : public Viewport, private Timer
{
public:
    MainComponent()
    {
        auto savedState = getAppProperties().getUserSettings()->getXmlValue (KEYNAME_DEVICE_SETTINGS);

        main.initialiseAudio(
                std::make_unique<AudioProcessorGraph>(),
                std::make_unique<AudioDeviceManager>(),
                std::make_unique<AudioProcessorPlayer>(),
                std::move(savedState)
        );

        setViewedComponent(&main);
        addAndMakeVisible(main);
        setBounds(0,0, 1920, 1080);

        
        auto image = ImageCache::getFromMemory (BinaryData::settings_png, BinaryData::settings_pngSize);

        settingsButton.setImages(false, true, false,
                image, 1.0f, Colours::grey, image, 1.0f, Colours::lightgrey,
                image, 1.0f, Colours::darkgrey);
        addAndMakeVisible(settingsButton);
        settingsButton.setBounds(getWidth() - 180, 80, 80, 80);

        startTimer(3);
    }

    ~MainComponent(){
        main.close();
    }

private:

    void move(int deltaX, int deltaY) {
        if (deltaX != 0  || deltaY != 0) {
            setViewPosition(getViewPositionX() - deltaX, getViewPositionY() - deltaY);
        }
    }

    void timerCallback() override {
        // scrolling
        auto pos = getMouseXYRelative();
        if (! getBounds().contains(pos))
            return;

        int deltaX = 0;
        int deltaY = 0;
        if (pos.x < 150)
            deltaX = (150 - pos.x) / 2;
        else if (pos.x > getWidth() - 150)
            deltaX = (getWidth() - 150 - pos.x) / 2;

        if (pos.y < 150)
            deltaY = (150 - pos.y) / 2;
        else if (pos.y > getHeight() - 150)
            deltaY = (getHeight() - 150 - pos.y) / 2;

        move(deltaX, deltaY);

        startTimer(3);
    }

    ImageButton settingsButton;

    EffectScene main;
    UndoManager undoManager;

};
