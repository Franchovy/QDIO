/*
  ==============================================================================

    MainComponent.cpp
    Created: 13 Apr 2020 4:18:45pm
    Author:  maxime

  ==============================================================================
*/
//#define DEBUG_APPEARANCE

#include "MainComponent.h"

MainComponent::MainComponent()
    : main()
    , effectTree(EFFECT_ID)
    , deviceManager(main.getDeviceManager())
    , audioGraph(main.getAudioGraph())
    , audioPlayer(main.getAudioPlayer())
    , settingsMenu(deviceManager)
{
    // EffectScene component
    setViewedComponent(&main, false);
    addAndMakeVisible(&main);

    // Set size to full screen
    auto appArea = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
#ifdef DEBUG_APPEARANCE
    // Make it smaller
    appArea = appArea.expanded(-900, -500);
#endif
    setBounds(appArea);

    main.setBounds(getBounds());
    main.view = getViewArea();

    auto image = ImageCache::getFromMemory (BinaryData::settings_png, BinaryData::settings_pngSize);
    settingsButton.setImages(false, true, false,
                             image, 1.0f, Colours::grey, image, 1.0f, Colours::lightgrey,
                             image, 1.0f, Colours::darkgrey);
    addAndMakeVisible(settingsButton);


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

    //============================================================================
    // Set up selection menus

    //
    effectSelectMenu.setText("Effects");
    effectSelectMenu.onChange = [=] {
        auto selectedIndex = effectSelectMenu.getSelectedItemIndex();
        if (selectedIndex >= 0) {
            auto effectName = effectSelectMenu.getItemText(selectedIndex);
            auto loadData = EffectLoader::loadEffect(effectName);
            main.menuCreateEffect(loadData);

            effectSelectMenu.setSelectedId(0, sendNotificationSync);
        }
    };

    updateEffectSelectMenu();


    //
    layoutMenu.setText("Layout");
    layoutMenu.onChange = [=] {
        auto selectedIndex = layoutMenu.getSelectedItemIndex();
        if (selectedIndex >= 0) {
            auto layoutName = layoutMenu.getItemText(selectedIndex);
            main.loadNewLayout(layoutName);

            effectSelectMenu.setSelectedId(0, sendNotificationSync);
        }
    };


    //
    toolBoxMenu.setText("Toolbox");

    addAndMakeVisible(layoutMenu);
    addAndMakeVisible(toolBoxMenu);
    addAndMakeVisible(effectSelectMenu);
    /*auto menu = effectSelectMenu.getRootMenu();
    populateEffectMenu(*menu);*/

    deviceManager.addChangeListener(this);

    startTimer(3);
}

MainComponent::~MainComponent() {
    main.storeState();

    auto audioState = main.getDeviceManager().createStateXml();

    getAppProperties().getUserSettings()->setValue (KEYNAME_DEVICE_SETTINGS, audioState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();

    EffectTreeBase::close();
}

bool MainComponent::keyPressed(const KeyPress &key) {
    std::cout << "key pressed" << newLine;
    return main.keyPressed(key);
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
    autoScroll(getMouseXYRelative().x, getMouseXYRelative().y, 50, 10);
/*
    if (pos.x < 150)

    else if (pos.x > getWidth() - 150)
        deltaX = (getWidth() - 150 - pos.x) / 2;

    if (pos.y < 150)
        deltaY = (150 - pos.y) / 2;
    else if (pos.y > getHeight() - 150)
        deltaY = (getHeight() - 150 - pos.y) / 2;

    move(deltaX, deltaY);
*/

    startTimer(3);
}

void MainComponent::changeListenerCallback(ChangeBroadcaster *source) {
    /*deviceManager.getCurrentAudioDevice()->stop();
    main.updateChannels();
    deviceManager.getCurrentAudioDevice()->start(&audioPlayer);*/
}
/*
void MainComponent::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) { 
    //main.setBounds(main.getBoundsInParent().expanded(wheel.deltaY));
}*/

void MainComponent::resized() {
    main.setBounds(getLocalBounds());
    settingsButton.setBounds(getWidth() - 180, 80, 80, 80);

    auto menu1 = FlexItem(settingsButton);
    auto menu2 = FlexItem(layoutMenu);
    auto menu3 = FlexItem(effectSelectMenu);
    auto menu4 = FlexItem(toolBoxMenu);

    menu1.height = menu2.height = menu3.height = menu4.height = 50;
    menu1.width = 50;
    menu2.width = menu3.width = menu4.width = 90;
    menu1.margin = menu2.margin = menu3.margin = menu4.margin = 10;

    FlexBox menus;
    menus.justifyContent = FlexBox::JustifyContent::flexStart;
    menus.flexDirection = FlexBox::Direction::row;
    menus.alignItems = FlexBox::AlignItems::flexStart;
    menus.items.add(menu1);
    menus.items.add(menu2);
    menus.items.add(menu3);
    menus.items.add(menu4);
    menus.performLayout(Rectangle<int>(100,100,400,60));
}

void MainComponent::populateEffectMenu(PopupMenu& menu) {
    /*auto test = std::unique_ptr<EffectSelectComponent>();
    menu.addCustomItem(1, std::move(test));*/

}

void MainComponent::updateEffectSelectMenu() {
    // EffectSelectMenu
    effectSelectMenu.clear(dontSendNotification);
    Image img = ImageCache::getFromMemory(BinaryData::settings_png, BinaryData::settings_pngSize);

    int i = 1;
    for (auto e : EffectLoader::getEffectsAvailable()) {
        effectSelectMenu.getRootMenu()->addItem(i++, e, true, false, img);
    }
}

void MainComponent::updateLayoutMenu() {
    layoutMenu.clear(dontSendNotification);
    Image img = ImageCache::getFromMemory(BinaryData::settings_png, BinaryData::settings_pngSize);

    int i = 1;
    for (auto e : EffectLoader::getLayoutsAvailable()) {
        layoutMenu.getRootMenu()->addItem(i++, e, true, false, img);
    }
}

void MainComponent::handleCommandMessage(int commandId) {
    if (commandId == 0) {
        std::cout << "Update menus" << newLine;
        updateEffectSelectMenu();
        updateLayoutMenu();
    }
}

