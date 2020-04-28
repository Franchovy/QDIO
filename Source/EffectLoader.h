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

const String KEYNAME_DEVICE_SETTINGS = "audioDeviceState"; // maybe move this?
const String KEYNAME_LOADED_EFFECTS = "loadedEffects";

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

        auto effectToOverWrite = effectLib.getChildWithName(storeEffect.getType());
        if (effectToOverWrite.isValid()) {
            std::cout << "Overwriting effect" << newLine;
            effectLib.removeChild(effectToOverWrite, nullptr);
        }
        
        effectLib.appendChild(storeEffect, nullptr);

        auto dataToStore = effectLib.createXml();

        getAppProperties().getUserSettings()->setValue(KEYNAME_EFFECT_LIBRARY, dataToStore.get());
        getAppProperties().getUserSettings()->save();
    }


    // store state code
    /*auto savedState = storeEffect(tree).createXml();

    std::cout << "Save state: " << savedState->toString() << newLine;
    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, savedState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();*/

private:
    static const String KEYNAME_EFFECT_LIBRARY;
};
