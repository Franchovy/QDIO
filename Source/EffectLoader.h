/*
  ==============================================================================

    EffectLoader.h
    Created: 28 Apr 2020 4:09:02pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

ApplicationProperties& getAppProperties();
ApplicationCommandManager& getCommandManager();

const String KEYNAME_APP_VERSION = "appVersion";
const String KEYNAME_DEVICE_SETTINGS = "audioDeviceState"; // maybe move this?
const String KEYNAME_LOADED_EFFECTS = "loadedEffects";
const String KEYNAME_LOADOUTS = "loadouts";
const String KEYNAME_CURRENT_LOADOUT = "currentLoadout";
const String KEYNAME_INITIAL_USE = "initialUse";

class EffectLoader {
public:
    // EffectLib
    static void saveEffect(ValueTree& storeEffect);
    static ValueTree loadEffect(String effectName);
    static StringArray getEffectsAvailable();
    static void clearEffect(String effectName);

    // Templates
    static void saveTemplate(ValueTree& newTemplate);
    static ValueTree loadTemplate(String templateName);
    static StringArray getTemplatesAvailable();
    static void clearTemplate(String templateName);

    //
    static void writeToFile(ValueTree data);
    static ValueTree loadFromFile();
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectLoader)
};
