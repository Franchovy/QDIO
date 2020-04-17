/*
  ==============================================================================

    MainComponent.cpp
    Created: 13 Apr 2020 4:18:45pm
    Author:  maxime

  ==============================================================================
*/

#include "MainComponent.h"

MainComponent::MainComponent()
    : main()
    , effectTree(EFFECT_ID) // TODO load if existing
    , deviceManager(main.getDeviceManager())
    , settingsMenu(deviceManager)
{
    // EffectScene component
    setViewedComponent(&main);
    addAndMakeVisible(main);
    setBounds(0,0, 1920, 1080);

    auto image = ImageCache::getFromMemory (BinaryData::settings_png, BinaryData::settings_pngSize);

    settingsButton.setImages(false, true, false,
                             image, 1.0f, Colours::grey, image, 1.0f, Colours::lightgrey,
                             image, 1.0f, Colours::darkgrey);
    addAndMakeVisible(settingsButton);
    settingsButton.setBounds(getWidth() - 180, 80, 80, 80);

    settingsMenu.setTopRightPosition(getWidth() - 40, 40);
    addChildComponent(settingsMenu);
    settingsButton.onClick = [=] {
        settingsMenu.setVisible(true);
        settingsButton.setVisible(false);
    };

    settingsMenu.getCloseButton().onClick = [=] {
        settingsMenu.setVisible(false);
        settingsButton.setVisible(true);
    };

    startTimer(3);
}

MainComponent::~MainComponent() {
    main.storeState();

    auto audioState = main.getDeviceManager().createStateXml();

    getAppProperties().getUserSettings()->setValue (KEYNAME_DEVICE_SETTINGS, audioState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();

    main.incReferenceCount();
    EffectTreeBase::close();
    main.decReferenceCountWithoutDeleting();
}

void MainComponent::move(int deltaX, int deltaY) {
    if (deltaX != 0  || deltaY != 0) {
        setViewPosition(getViewPositionX() - deltaX, getViewPositionY() - deltaY);
    }
}

void MainComponent::timerCallback() {
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