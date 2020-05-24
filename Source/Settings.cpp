/*
  ==============================================================================

    Settings.cpp
    Created: 13 Apr 2020 6:22:39am
    Author:  maxime

  ==============================================================================
*/

#include "Settings.h"

AudioDeviceManager* SettingsComponent::deviceManager = nullptr;
AudioDeviceManager::AudioDeviceSetup SettingsComponent::setup;


/*
deviceSelectorComponent.closeButton.onClick = [=]{
    deviceSelectorComponent.setVisible(false);
    auto audioState = deviceManager->createStateXml();

    getAppProperties().getUserSettings()->setValue (KEYNAME_DEVICE_SETTINGS, audioState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();
};*/
