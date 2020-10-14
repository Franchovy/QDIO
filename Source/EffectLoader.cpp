/*
  ==============================================================================

    EffectLoader.cpp
    Created: 28 Apr 2020 4:09:02pm
    Author:  maxime

  ==============================================================================
*/

#include "EffectLoader.h"

void EffectLoader::saveEffect(ValueTree &storeEffect) {
    ValueTree effectLib("EffectLib");

    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);
    if (loadedEffectsData != nullptr) {
        effectLib = ValueTree::fromXml(*loadedEffectsData);
    }

    auto effectToOverWrite = effectLib.getChildWithProperty("name",
                                                            storeEffect.getProperty("name").toString());

    if (effectToOverWrite.isValid()) {
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
        effectLib.removeChild(effectToRemove, nullptr);
    }

    auto dataToStore = effectLib.createXml();

    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, dataToStore.get());
    getAppProperties().getUserSettings()->save();
}

void EffectLoader::saveTemplate(ValueTree &newTemplate) {
    ValueTree effectLib("Templates");

    auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADOUTS);
    if (loadedEffectsData != nullptr) {
        effectLib = ValueTree::fromXml(*loadedEffectsData);
    }

    auto templateToOverWrite = effectLib.getChildWithProperty("name",
                                                              newTemplate.getProperty("name").toString());

    if (templateToOverWrite.isValid()) {
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
    FileChooser fileChooser("Load file, files or effect folder");
    auto confirmed = fileChooser.browseForMultipleFilesOrDirectories();

    if (confirmed == 0) {
        return ValueTree();
    } else {
        ValueTree loadData("data");

        for (auto result : fileChooser.getResults()) {
            if (result.isDirectory()) {
                for (auto file : result.findChildFiles(File::findFiles, true)) {
                    File inputFile(fileChooser.getResult());

                    FileInputStream in(inputFile);
                    ValueTree effectData;
                    effectData = ValueTree::readFromStream(in);
                    loadData.appendChild(effectData, nullptr);
                }
            } else {
                File inputFile(result);

                FileInputStream in(inputFile);
                ValueTree effectData;
                effectData = ValueTree::readFromStream(in);
                loadData.appendChild(effectData, nullptr);
            }
        }

        return loadData;
    }
}
