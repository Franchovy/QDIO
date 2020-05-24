/*
  ==============================================================================

    Settings.h
    Created: 13 Apr 2020 6:22:39am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SettingsComponent : public Component
{
public:
    explicit SettingsComponent(AudioDeviceManager& dm) : deviceSelectorMenu(dm, 0, 2, 0, 2, false, false, true, false)
    {
        addAndMakeVisible(closeButton);
        addAndMakeVisible(deviceSelectorMenu);

        deviceManager = &dm;
        dm.getAudioDeviceSetup(setup);

        auto image = ImageCache::getFromMemory (BinaryData::close_png, BinaryData::close_pngSize);

        closeButton.setImages(false, true, false,
                                 image, 1.0f, Colours::grey, image, 1.0f, Colours::lightgrey,
                                 image, 1.0f, Colours::darkgrey);

        setSize(800, 450);
    }


    ImageButton& getCloseButton() { return closeButton; }

    void paint(Graphics& g) override {
        g.setColour(Colours::whitesmoke);
        g.drawRoundedRectangle(outline, 5, 1);
    }

    void resized() override {
        outline.setBounds(0, 0, getWidth(), getHeight());
        closeButton.setBounds(getWidth() - 30, 10, 20, 20);
        deviceSelectorMenu.setBounds(10, 30, getWidth() - 10, getHeight() - 10);
    }

    static StringArray getDevicesList(bool isInput) {
        auto currentDeviceType = deviceManager->getCurrentDeviceTypeObject();
        auto currentAudioDevice = deviceManager->getCurrentAudioDevice();

        return StringArray(currentDeviceType->getDeviceNames(isInput));
    }

    static int getCurrentDeviceIndex(bool isInput) {
        auto currentDeviceType = deviceManager->getCurrentDeviceTypeObject();
        auto currentAudioDevice = deviceManager->getCurrentAudioDevice();

        return currentDeviceType->getIndexOfDevice(currentAudioDevice, isInput);
    }

    static void setCurrentDevice(bool isInput, int index) {
        auto newDevice = deviceManager->getCurrentDeviceTypeObject()->getDeviceNames(isInput)[index];
        setup = deviceManager->getAudioDeviceSetup();
        if (isInput) {
            setup.inputDeviceName = newDevice;
            setup.useDefaultInputChannels = true;
        } else {
            setup.outputDeviceName = newDevice;
            setup.useDefaultOutputChannels = true;
        }
       
        deviceManager->setAudioDeviceSetup(setup, true);
    }

private:
    Rectangle<float> outline;
    AudioDeviceSelectorComponent deviceSelectorMenu;
    ImageButton closeButton;

    static AudioDeviceManager::AudioDeviceSetup setup;
    static AudioDeviceManager* deviceManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComponent)
};

