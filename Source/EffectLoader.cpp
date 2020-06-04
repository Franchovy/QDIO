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
        //std::cout << "Stored effects:" << newLine << effectLib.toXmlString() << newLine;
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

void EffectLoader::saveTemplate(ValueTree &newTemplate) {

    std::cout << "Saving template: " << newTemplate.getType().toString() << newLine;
    std::cout << newTemplate.toXmlString() << newLine;

    ValueTree effectLib("Templates");

    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedEffectsData != nullptr) {
        effectLib = ValueTree::fromXml(*loadedEffectsData);
    }

    auto templateToOverWrite = effectLib.getChildWithProperty("name",
                                                              newTemplate.getProperty("name").toString());

    if (templateToOverWrite.isValid()) {
        std::cout << "Overwriting template" << newLine;
        effectLib.removeChild(templateToOverWrite, nullptr);
    }

    effectLib.appendChild(newTemplate, nullptr);

    auto dataToStore = effectLib.createXml();
    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADOUTS, dataToStore.get());
    getAppProperties().getUserSettings()->save();
}

ValueTree EffectLoader::loadTemplate(String templateName) {
    ValueTree templates("Templates");

    auto loadedTemplatesData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedTemplatesData != nullptr) {
        templates = ValueTree::fromXml(*loadedTemplatesData);
        //std::cout << "Stored templates:" << newLine << templates.toXmlString() << newLine;
    }

    auto templateToLoad = templates.getChildWithProperty("name", templateName);
    if (templateToLoad.isValid()) {
        return templateToLoad;
    } else {
        return ValueTree();
    }
}

StringArray EffectLoader::getTemplatesAvailable() {
    StringArray templatesList;

    std::cout << "Effects available: " << newLine;

    ValueTree templates("Templates");
    auto loadedTemplatesData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedTemplatesData != nullptr) {
        templates = ValueTree::fromXml(*loadedTemplatesData);

        for (int i = 0; i < templates.getNumChildren(); i++) {
            std::cout << templates.getChild(i).getProperty("name").toString() << newLine;
            templatesList.add(templates.getChild(i).getProperty("name").toString());
        }
    }

    return templatesList;
}

void EffectLoader::clearTemplate(String templateName) {
    ValueTree templates("Templates");

    auto loadedTemplatesData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedTemplatesData != nullptr) {
        templates = ValueTree::fromXml(*loadedTemplatesData);
    }

    auto templateToRemove = templates.getChildWithProperty("name", templateName);

    if (templateToRemove.isValid()) {
        std::cout << "Removing effect" << newLine;
        templates.removeChild(templateToRemove, nullptr);
    }

    auto dataToStore = templates.createXml();

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
