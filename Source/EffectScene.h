/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/



#pragma once

#include <JuceHeader.h>
#include <BinaryData.h>

#include "Effect.h"
#include "TypeHandlers.h"

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

const String KEYNAME_DEVICE_SETTINGS = "audioDeviceState";


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

    void mouseDown(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    bool keyPressed(const KeyPress &key) override;

    void deleteEffect(Effect* e);

    AudioDeviceManager& getDeviceManager() { return deviceManager; }

private:
    //==============================================================================
    // Audio shit
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;

    Image bg;

    PopupMenu mainMenu;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectScene)
};

