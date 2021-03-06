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
    : effectScene()
    , effectTree(EFFECT_ID)
    , audioSystem()
    , settingsMenu(*audioSystem.getDeviceManager())
{
    auto& desktop = Desktop::getInstance();
    // Set size to full screen
    auto mainDisplay = desktop.getDisplays().getMainDisplay();
    auto appArea = mainDisplay.userArea;

    float scaleRatio = appArea.getWidth() / 2000.f;
    
    if (scaleRatio < 1) {
        desktop.setGlobalScaleFactor(0.5f);
    } else if (scaleRatio > 1.5) {
        desktop.setGlobalScaleFactor(1.5f);
    }

    setBounds(appArea);

    // EffectScene component
    setViewedComponent(&effectScene, false);
    addAndMakeVisible(&effectScene);
    effectScene.setBounds(getLocalBounds());


#ifdef DEBUG_APPEARANCE
    // Make it smaller
    appArea = appArea.expanded(-900, -500);
#endif


    auto image = ImageCache::getFromMemory (BinaryData::settings_png, BinaryData::settings_pngSize);
    settingsButton.setImages(false, true, false,
                             image, 1.0f, Colours::grey, image, 1.0f, Colours::lightgrey,
                             image, 1.0f, Colours::darkgrey);
    addAndMakeVisible(settingsButton);

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
    effectSelectMenu.onChange = [=] {
        auto selectedIndex = effectSelectMenu.getSelectedItemIndex();
        if (selectedIndex >= 0) {
            auto effectName = effectSelectMenu.getItemText(selectedIndex);
            auto loadData = EffectLoader::loadEffect(effectName);
            //effectScene.menuCreateEffect(loadData);

            effectSelectMenu.setText("Effects");
        }
    };

    updateEffectSelectMenu();
    effectSelectMenu.setText("Effects");

    //
    templateMenu.onChange = [=] {
        auto selectedIndex = templateMenu.getSelectedItemIndex();
        if (selectedIndex >= 0) {
            auto templateName = templateMenu.getItemText(selectedIndex);
            //effectScene.loadNewTemplate(templateName);

            templateMenu.setText("Template");
        }
    };

    updateTemplateMenu();
    templateMenu.setText("Template");

    //
    toolBoxMenu.setText("ToolBox");

    toolBoxMenu.onChange = [=] {
        auto selectedIndex = toolBoxMenu.getSelectedItemIndex();
        if (selectedIndex >= 0) {
            //effectScene.createProcessor(selectedIndex);

            toolBoxMenu.setText("ToolBox");
        }
    };

    populateToolBoxMenu();

    addAndMakeVisible(templateMenu);
    addAndMakeVisible(toolBoxMenu);
    addAndMakeVisible(effectSelectMenu);
    /*auto menu = effectSelectMenu.getRootMenu();
    populateEffectMenu(*menu);*/

    //deviceManager.addChangeListener(this);

    startTimer(3);
}

MainComponent::~MainComponent() {
    //effectScene.storeState();

    auto audioState = audioSystem.getDeviceManager()->createStateXml();

    getAppProperties().getUserSettings()->setValue (KEYNAME_DEVICE_SETTINGS, audioState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();
}

bool MainComponent::keyPressed(const KeyPress &key) {
    std::cout << "key pressed" << newLine;
        
    return effectScene.keyPressed(key);
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
    effectScene.updateChannels();
    deviceManager.getCurrentAudioDevice()->start(&audioPlayer);*/
}
/*
void MainComponent::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) { 
    //effectScene.setBounds(effectScene.getBoundsInParent().expanded(wheel.deltaY));
}*/

void MainComponent::resized() {
    effectScene.setBounds(getLocalBounds());
    settingsButton.setBounds(getWidth() - 180, 80, 80, 80);

    auto menu1 = FlexItem(settingsButton);
    auto menu2 = FlexItem(templateMenu);
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

    settingsMenu.setTopLeftPosition(40, settingsButton.getY() + 100);
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
    effectSelectMenu.setText("Effects");
}

void MainComponent::updateTemplateMenu() {
    templateMenu.clear(dontSendNotification);
    Image img = ImageCache::getFromMemory(BinaryData::settings_png, BinaryData::settings_pngSize);

    int i = 1;
    for (auto e : EffectLoader::getTemplatesAvailable()) {
        if (e.compare("default") != 0) { // Don't add default (working) template to list
            templateMenu.getRootMenu()->addItem(i++, e, true, false, img);
        }
    }
    templateMenu.setText("Template");
}

void MainComponent::handleCommandMessage(int commandId) {
    if (commandId == 0) {
        std::cout << "Update menus" << newLine;
        updateEffectSelectMenu();
        updateTemplateMenu();
    }
}

void MainComponent::populateToolBoxMenu() {
    int i = 0;
    /*for (auto item : effectScene.getProcessorNames()) {
        toolBoxMenu.addItem(item, ++i);
    }*/
}

