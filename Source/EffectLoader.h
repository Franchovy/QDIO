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

class EffectLoader {
public:
    static void saveEffect(ValueTree& storeEffect) {
        std::cout << "Saving effect: " << storeEffect.getType().toString() << newLine;
        std::cout << storeEffect.toXmlString() << newLine;

        ValueTree effectLib("EffectLib");

        auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_EFFECT_LIBRARY);
        if (loadedEffectsData != nullptr) {
            effectLib = ValueTree::fromXml(*loadedEffectsData);
        }

        auto effectToOverWrite = effectLib.getChildWithProperty("name",
                storeEffect.getProperty("name").toString());

        if (effectToOverWrite.isValid()) {
            std::cout << "Overwriting effect" << newLine;
            effectLib.removeChild(effectToOverWrite, nullptr);
        }

        effectLib.appendChild(storeEffect, nullptr);

        auto dataToStore = effectLib.createXml();

        getAppProperties().getUserSettings()->setValue(KEYNAME_EFFECT_LIBRARY, dataToStore.get());
        getAppProperties().getUserSettings()->save();
    }

    static ValueTree loadEffect(String effectName) {
        ValueTree effectLib("EffectLib");

        auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_EFFECT_LIBRARY);
        if (loadedEffectsData != nullptr) {
            effectLib = ValueTree::fromXml(*loadedEffectsData);
            std::cout << "Stored effects:" << newLine << effectLib.toXmlString() << newLine;
        }

        auto effectToLoad = effectLib.getChildWithProperty("name", effectName);
        if (effectToLoad.isValid()) {
            return effectToLoad;
        } else {
            return ValueTree();
        }
    }

    // TODO
    // static String list getEffects()
    static StringArray getEffectsAvailable() {
        StringArray effectList;

        std::cout << "Effects available: " << newLine;

        ValueTree effectLib("EffectLib");
        auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_EFFECT_LIBRARY);
        if (loadedEffectsData != nullptr) {
            effectLib = ValueTree::fromXml(*loadedEffectsData);

            for (int i = 0; i < effectLib.getNumChildren(); i++) {
                std::cout << effectLib.getChild(i).getProperty("name").toString() << newLine;
                effectList.add(effectLib.getChild(i).getProperty("name").toString());
            }
        }

        return effectList;
    }

    static void clearEffect(String effectName) {
        ValueTree effectLib("EffectLib");

        auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_EFFECT_LIBRARY);
        if (loadedEffectsData != nullptr) {
            effectLib = ValueTree::fromXml(*loadedEffectsData);
        }

        auto effectToRemove = effectLib.getChildWithProperty("name", effectName);

        if (effectToRemove.isValid()) {
            std::cout << "Removing effect" << newLine;
            effectLib.removeChild(effectToRemove, nullptr);
        }

        auto dataToStore = effectLib.createXml();

        getAppProperties().getUserSettings()->setValue(KEYNAME_EFFECT_LIBRARY, dataToStore.get());
        getAppProperties().getUserSettings()->save();
    }

private:
    static const String KEYNAME_EFFECT_LIBRARY;
};
