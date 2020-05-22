/*
  ==============================================================================

    EffectLoader.cpp
    Created: 28 Apr 2020 4:09:02pm
    Author:  maxime

  ==============================================================================
*/

#include "EffectLoader.h"

void EffectLoader::saveEffect(ValueTree &storeEffect) {
    std::cout << "Saving effect: " << storeEffect.getType().toString() << newLine;
    std::cout << storeEffect.toXmlString() << newLine;

    ValueTree effectLib("EffectLib");

    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);
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
    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, dataToStore.get());
    getAppProperties().getUserSettings()->save();
}

ValueTree EffectLoader::loadEffect(String effectName) {
    ValueTree effectLib("EffectLib");

    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);
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

StringArray EffectLoader::getEffectsAvailable() {
    StringArray effectList;

    std::cout << "Effects available: " << newLine;

    ValueTree effectLib("EffectLib");
    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);
    if (loadedEffectsData != nullptr) {
        effectLib = ValueTree::fromXml(*loadedEffectsData);

        for (int i = 0; i < effectLib.getNumChildren(); i++) {
            std::cout << effectLib.getChild(i).getProperty("name").toString() << newLine;
            effectList.add(effectLib.getChild(i).getProperty("name").toString());
        }
    }

    return effectList;
}

void EffectLoader::clearEffect(String effectName) {
    ValueTree effectLib("EffectLib");

    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);
    if (loadedEffectsData != nullptr) {
        effectLib = ValueTree::fromXml(*loadedEffectsData);
    }

    auto effectToRemove = effectLib.getChildWithProperty("name", effectName);

    if (effectToRemove.isValid()) {
        std::cout << "Removing effect" << newLine;
        effectLib.removeChild(effectToRemove, nullptr);
    }

    auto dataToStore = effectLib.createXml();

    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, dataToStore.get());
    getAppProperties().getUserSettings()->save();
}

void EffectLoader::saveLayout(ValueTree &layout) {

    std::cout << "Saving layout: " << layout.getType().toString() << newLine;
    std::cout << layout.toXmlString() << newLine;

    ValueTree effectLib("Layouts");

    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedEffectsData != nullptr) {
        effectLib = ValueTree::fromXml(*loadedEffectsData);
    }

    auto layoutToOverWrite = effectLib.getChildWithProperty("name",
                                                            layout.getProperty("name").toString());

    if (layoutToOverWrite.isValid()) {
        std::cout << "Overwriting layout" << newLine;
        effectLib.removeChild(layoutToOverWrite, nullptr);
    }

    effectLib.appendChild(layout, nullptr);

    auto dataToStore = effectLib.createXml();
    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADOUTS, dataToStore.get());
    getAppProperties().getUserSettings()->save();
}

ValueTree EffectLoader::loadLayout(String layoutName) {
    ValueTree layouts("Layouts");

    auto loadedLayoutsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedLayoutsData != nullptr) {
        layouts = ValueTree::fromXml(*loadedLayoutsData);
        std::cout << "Stored layouts:" << newLine << layouts.toXmlString() << newLine;
    }

    auto layoutToLoad = layouts.getChildWithProperty("name", layoutName);
    if (layoutToLoad.isValid()) {
        return layoutToLoad;
    } else {
        return ValueTree();
    }
}

StringArray EffectLoader::getLayoutsAvailable() {
    StringArray layoutsList;

    std::cout << "Effects available: " << newLine;

    ValueTree layouts("Layouts");
    auto loadedLayoutsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedLayoutsData != nullptr) {
        layouts = ValueTree::fromXml(*loadedLayoutsData);

        for (int i = 0; i < layouts.getNumChildren(); i++) {
            std::cout << layouts.getChild(i).getProperty("name").toString() << newLine;
            layoutsList.add(layouts.getChild(i).getProperty("name").toString());
        }
    }

    return layoutsList;
}

void EffectLoader::clearLayout(String layoutName) {
    ValueTree layouts("Layouts");

    auto loadedLayoutsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedLayoutsData != nullptr) {
        layouts = ValueTree::fromXml(*loadedLayoutsData);
    }

    auto layoutToRemove = layouts.getChildWithProperty("name", layoutName);

    if (layoutToRemove.isValid()) {
        std::cout << "Removing effect" << newLine;
        layouts.removeChild(layoutToRemove, nullptr);
    }

    auto dataToStore = layouts.createXml();

    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADOUTS, dataToStore.get());
    getAppProperties().getUserSettings()->save();
}

void EffectLoader::writeToFile(ValueTree data) {
    FileChooser fileChooser("Save file");
    auto result = fileChooser.browseForFileToSave(true);

    if (result == 0) {
        return;
    } else {
        File outputFile(fileChooser.getResult());

        FileOutputStream out(outputFile);
        data.writeToStream(out);
    }
}

ValueTree EffectLoader::loadFromFile() {
    FileChooser fileChooser("Load file");
    auto result = fileChooser.browseForFileToOpen();

    if (result == 0) {
        return ValueTree();
    } else {
        File inputFile(fileChooser.getResult());

        FileInputStream in(inputFile);
        ValueTree loadData;
        loadData = ValueTree::readFromStream(in);
        return loadData;
    }
}
