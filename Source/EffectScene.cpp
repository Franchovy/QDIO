#include "EffectScene.h"

const Identifier EffectScene::IDs::DeviceManager = "deviceManager";
EffectScene* EffectScene::instance = nullptr;

//==============================================================================
EffectScene::EffectScene()
        : EffectTreeBase()
        , MenuItem(1)
        , tree(this)
{
    setComponentID("EffectScene");
    setName("EffectScene");

    //================================================================================
    // LOADING PARAMETERS

    // If this is the Initial Use Case
    loadInitialCase = getAppProperties().getUserSettings()->getBoolValue(KEYNAME_INITIAL_USE, true);


    // Don't load anything if this isn't the right version.

    String currentVersion = "1.5";
    std::cout << "App version: " << currentVersion << newLine;
    auto appVersion = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_APP_VERSION);

    if (appVersion != nullptr && appVersion->hasAttribute("version")) {
        std::cout << "Load version: " << appVersion->getStringAttribute("version") << newLine;
        if (appVersion->getStringAttribute("version") == currentVersion) {
            dontLoad = false;
        } else {
            dontLoad = true;

        }
    } else {
        dontLoad = true;
    }

    loadInitialCase = dontLoad;
    bool dontLoadDevices = dontLoad;

    dontLoad = true;
    //bool dontLoadDevices = false;

    std::cout << "Loading state? " << (dontLoad ? "false" : "true") << newLine;

    //=======================================================================
    //

    setBufferedToImage(true);
    //setPaintingIsUnclipped(false);
    setRepaintsOnMouseActivity(false);

    hoverable = false;

    // Set up static members
    instance = this;
    EffectTreeBase::effectScene = this;

    EffectTreeBase::audioGraph = &audioGraph;
    EffectTreeBase::processorPlayer = &processorPlayer;
    EffectTreeBase::deviceManager = &deviceManager;

    audioGraph.enableAllBuses();
   
    if (dontLoadDevices) {
        //getAppProperties().getUserSettings()->getXmlValue(KEYNAME_DEVICE_SETTINGS).get()
        deviceManager.initialiseWithDefaultDevices(2, 2);
    } else {
        deviceManager.initialise(2, 2, getAppProperties().getUserSettings()->getXmlValue(KEYNAME_DEVICE_SETTINGS).get(),
                                 true);

    }
    deviceManager.addAudioCallback(&processorPlayer);
    deviceManager.addChangeListener(this);
    processorPlayer.setProcessor(&audioGraph);

    setMouseClickGrabsKeyboardFocus(true);

    // Drag Line GUI
    addChildComponent(lasso);



#define BACKGROUND_IMAGE
    bg = ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);


    // Main component popup menu
    //setupCreateEffectMenu();


    //==========================================================================
    // Load starting state

    if (dontLoad) {
        // Clear all settings
        getAppProperties().getUserSettings()->clear();

        // Update load app version
        XmlElement newVersion("version");
        newVersion.setAttribute("version", currentVersion);
        getAppProperties().getUserSettings()->setValue(KEYNAME_APP_VERSION, &newVersion);
        getAppProperties().getUserSettings()->save();
    }

    startTimer(500);
}

void EffectScene::timerCallback() {
    stopTimer();
    // Load
    if (loadInitialCase) {
        // Load initial layout
        auto initialLayout = ValueTree::readFromData(BinaryData::BasicInOutLayout, BinaryData::BasicInOutLayoutSize);

        jassert(initialLayout.getType() == Identifier(EFFECTSCENE_ID));
        EffectLoader::saveLayout(initialLayout);

        auto name = initialLayout.getProperty("name");
        tree.loadLayout(name);

        std::cout << "Initial layout load!" << newLine;

        getAppProperties().getUserSettings()->setValue(KEYNAME_INITIAL_USE, true);

        // Load effects and layouts to EffectLoader
        auto data = ValueTree::readFromData(BinaryData::Guitar_Rig, BinaryData::Guitar_RigSize);
        EffectLoader::saveLayout(data);

        data = ValueTree::readFromData(BinaryData::GuitarInputEffect, BinaryData::GuitarInputEffectSize);
        EffectLoader::saveEffect(data);

        data = ValueTree::readFromData(BinaryData::OutputAmpEffect, BinaryData::OutputAmpEffectSize);
        EffectLoader::saveEffect(data);

        data = ValueTree::readFromData(BinaryData::MegaGrunge, BinaryData::MegaGrungeSize);
        EffectLoader::saveEffect(data);

        data = ValueTree::readFromData(BinaryData::SuperDelay, BinaryData::SuperDelaySize);
        EffectLoader::saveEffect(data);

        // Refresh effect and layout menus
        postCommandMessage(0);
    } else if (! dontLoad) {
        // Load layout
        appState = loading;

        // Load previously loaded layout
        String name = getAppProperties().getUserSettings()->getValue(KEYNAME_CURRENT_LOADOUT);
        std::cout << "Load layout: " << name << newLine;
        tree.loadLayout(name);

        undoManager.clearUndoHistory();
        appState = neutral;
    }
}


void EffectScene::changeListenerCallback(ChangeBroadcaster *source) {
    if (source == &deviceManager) {
        // DeviceManager change
        std::cout << "AudioDeviceManager change" << newLine;

    }
}


EffectScene::~EffectScene()
{
    instance = nullptr;
    //todo remove this ptr from EffectTree before destructor.

    /*audioGraph.clear();
    jassert(! audioGraph.getParameters().isEmpty());*/

    processorPlayer.setProcessor(nullptr);
    deviceManager.closeAudioDevice();

    undoManager.clearUndoHistory();
}

/*Array<PopupMenu::Item> EffectScene::setupCreateEffectMenu() {

    Array<PopupMenu::Item> returnArray;
    std::unique_ptr<PopupMenu> createEffectMenu = std::make_unique<PopupMenu>();

    PopupMenu::Item empty("Empty Effect");
    empty.setAction( std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Effect");
                auto effectVT = tree.newEffect("Effect", getMouseXYRelative());
                tree.createEffect(effectVT);
            }
    ));
    returnArray.add(empty);

    PopupMenu::Item input("Input Device");
    input.setAction(std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Input Effect");
                auto effectVT = tree.newEffect("Input Device", getMouseXYRelative(), 0);
                tree.createEffect(effectVT);
            }
    ));
    returnArray.add(input);

    PopupMenu::Item output("Output Device");
    output.setAction(std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Output Effect");
                auto effectVT = tree.newEffect("Output Device", getMouseXYRelative(), 1);
                tree.createEffect(effectVT);
            }
    ));
    returnArray.add(output);

    PopupMenu::Item distortion("Distortion Effect");
    distortion.setAction(std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Distortion Effect");
                auto effectVT = tree.newEffect("Distortion Effect", getMouseXYRelative(), 2);
                tree.createEffect(effectVT);
            }
    ));
    returnArray.add(distortion);

    PopupMenu::Item delay("Delay Effect");
    delay.setAction(std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Delay Effect");
                auto effectVT = tree.newEffect("Delay Effect", getMouseXYRelative(), 3);
                tree.createEffect(effectVT);
            }
    ));
    returnArray.add(delay);

    PopupMenu::Item reverb("Reverb Effect");
    reverb.setAction(std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Reverb Effect");
                auto effectVT = tree.newEffect("Reverb Effect", getMouseXYRelative(), 4);
                tree.createEffect(effectVT);
            }
    ));
    returnArray.add(reverb);

    *//*PopupMenu::Item createEffectMenuItem("Create effect..");
    createEffectMenuItem.subMenu = std::move(createEffectMenu);
    addMenuItem(0, createEffectMenuItem);*//*

    return returnArray;
}*/

void EffectScene::createProcessor(int processorID) {
    auto newEffect = tree.newEffect(tree.getProcessorName(processorID), getMouseXYRelative(), processorID);
    tree.loadEffect(newEffect);
}



//==============================================================================
void EffectScene::paint (Graphics& g)
{
#ifdef BACKGROUND_IMAGE

    //g.setTiledImageFill(bgTile, 0, 0, 1.0f);
    //g.fillRect(getBoundsInParent());

    g.drawImage(bg,getBounds().toFloat());

#else
    g.fillAll (Colour(30, 35, 40));
    
    // bg logo
    g.setOpacity(0.8f);
    g.setColour(Colours::ghostwhite);
    g.drawEllipse(getWidth()/2 - 250, getHeight()/2 - 250, 500, 500, 0.2f);
    
    g.setOpacity(0.3f);
    g.setFont(30);
    
    g.drawText("QDIO",getWidth()/2-50, getHeight()/2-50, 100, 100, Justification::centred);
#endif

    Component::paint(g);
}

void EffectScene::resized()
{
    //tileDelta()
    //repaint();
}

void EffectScene::mouseDown(const MouseEvent &event) {

    // EffectScene
    if (event.originalComponent == this && event.mods.isLeftButtonDown()) {
        lasso.setVisible(true);
        lasso.beginLasso(event, this);
    }
    // Resizer
    else if (dynamic_cast<Resizer*>(event.originalComponent)) {
        undoManager.beginNewTransaction("resize");
        return;
    }
}

void EffectScene::mouseDrag(const MouseEvent &event) {
    if (lasso.isVisible()) {
        lasso.dragLasso(event);
    }
}

void EffectScene::mouseUp(const MouseEvent &event) {
    if (lasso.isVisible()) {
        lasso.endLasso();
    }


    if (event.originalComponent == this) {
        // Click on effectscene
        if (event.getDistanceFromDragStart() < 10)
        {
            // Deselect on effectscene click
            if (event.mods.isLeftButtonDown() && ! event.mods.isCtrlDown()
                                                 && ! event.mods.isShiftDown()) {
                // Don't deselect if shift or ctrl is held.
                deselectAll();
            }
            // Right mouse click opens menu
            else if (event.mods.isRightButtonDown()
                    || event.mods.isCtrlDown())
            {
                callMenu(0);
            }
        }
    } else if (auto effect = dynamic_cast<Effect*>(event.originalComponent)) {
        // Mouse click
        if (event.getDistanceFromDragStart() < 10) {
            // Call menu
            if (event.mods.isRightButtonDown() ||
                event.mods.isCtrlDown()) {
                // open menu
                //menuPos = event.getPosition();
                if (effect->isInEditMode()) {
                    effect->callMenu(effect->editMenu);
                } else {
                    effect->callMenu(effect->menu);
                }
            }
        }
    }
}

bool EffectScene::keyPressed(const KeyPress &key)
{
#define DEBUG_UTILITIES
#ifdef DEBUG_UTILITIES
    if (key.getKeyCode() == 's') {
        std::cout << "Audiograph status: " << newLine;
        for (auto node : audioGraph.getNodes()) {
            if (node != nullptr) {
                std::cout << "node: " << node->nodeID.uid << newLine;
                std::cout << node->getProcessor()->getName() << newLine;
            } else {
                std::cout << "Null node." << newLine;
            }
        }
    }
    if (key.getKeyCode() == 'p') {
        std::cout << "Main Position: " << getMouseXYRelative().toString() << newLine;
        std::cout << "Relative Position: " << getComponentAt(getMouseXYRelative())->getLocalPoint(this, getMouseXYRelative()).toString() << newLine;
    }
    /*if (key.getKeyCode() == 'e') {
        if (auto e = dynamic_cast<EffectTreeBase*>(getComponentAt(getMouseXYRelative()))) {
            auto tree = e->getTree();
            std::cout << "Properties: " << newLine;
            for (int i = 0; i < tree.getNumProperties(); i++) {
                auto property = tree.getPropertyName(i);
                std::cout << property.toString() << ": " << tree.getProperty(property).toString() << newLine;
            }
            if (tree.getNumChildren() > 0) {
                std::cout << "Children: " << newLine;
                for (int i = 0; i < tree.getNumChildren(); i++) {
                    auto child = tree.getChild(i);

                    std::cout << child.getType().toString() << newLine;
                }
            }
        }
    }*/
    if (key.getKeyCode() == 'l') {
        std::cout << "Available effects: " << newLine;
        for (auto e : EffectLoader::getEffectsAvailable()) {
            std::cout << e << newLine;
        }
    }
    if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'l') {
        std::cout << "Saved effects: " << newLine;
        for (auto e : EffectLoader::getEffectsAvailable()) {
            std::cout << "Effect: " << e << newLine;
            std::cout << EffectLoader::loadEffect(e).toXmlString() << newLine;
        }
    }
    if (key.getKeyCode() == '=') {
        auto scaleFactor = Desktop::getInstance().getGlobalScaleFactor();
        scaleFactor += 0.1f;
        Desktop::getInstance().setGlobalScaleFactor(scaleFactor);
        repaint();
        return true;
    } else if (key.getKeyCode() == '-') {
        auto scaleFactor = Desktop::getInstance().getGlobalScaleFactor();
        scaleFactor -= 0.1f;
        Desktop::getInstance().setGlobalScaleFactor(scaleFactor);
        repaint();
        return true;
    }
#endif

    if (key.getKeyCode() == KeyPress::spaceKey) {
        if (deviceManager.getCurrentAudioDevice()->isPlaying()) {
            deviceManager.getCurrentAudioDevice()->stop();
        } else {
            deviceManager.getCurrentAudioDevice()->start(&processorPlayer);
        }
    }

    if (key.getKeyCode() == KeyPress::deleteKey || key.getKeyCode() == KeyPress::backspaceKey) {
        undoManager.beginNewTransaction("Delete");

        for (const auto& selectedItem : getSelected()) {
            if (dynamic_cast<Effect*>(selectedItem)) {
                tree.remove(selectedItem);
            }
        }

        for (const auto& selectedItem : getSelected()) {
            if (dynamic_cast<ConnectionLine*>(selectedItem)) {
                tree.remove(selectedItem);
            }
        }

        for (const auto& selectedItem : getSelected()) {
            if (! dynamic_cast<ConnectionLine*>(selectedItem) &&
                    ! dynamic_cast<Effect*>(selectedItem))
            {
                tree.remove(selectedItem);
            }
        }

        deselectAll();
    }

    if (key.getKeyCode() == KeyPress::escapeKey)
    {
        deselectAll();
    }

    // CTRL
    if (key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown()) {
        if (key.getKeyCode() == 'z' || (key.getKeyCode() == 'Z' && ! key.getModifiers().isShiftDown())) {
            std::cout << "Undo: " << undoManager.getUndoDescription() << newLine;
            undoManager.undo();
        } else if (key.getKeyCode() == 'Z' && key.getModifiers().isShiftDown()) {
            std::cout << "Redo: " << undoManager.getRedoDescription() << newLine;
            undoManager.redo();
        }

        if (key.getKeyCode() == 's' || key.getKeyCode() == 'S') {
            if (selected.getItemArray().size() == 1) {
                auto item = selected.getSelectedItem(0);

                if (auto effect = dynamic_cast<Effect*>(item.get())) {
                    String name = item->getName();

                    int result = callSaveEffectDialog(name);

                    if (result == 1) {
                        auto effectTree = tree.getTree(effect);

                        // Set name to whatever was entered
                        effect->setName(name);
                        effectTree.setProperty("name", name, nullptr);

                        auto data = tree.storeEffect(tree.getTree(effect));
                        EffectLoader::saveEffect(data);

                        std::cout << "Saved Effect: " << name << newLine;
                        postCommandMessage(0);
                    }
                }
            } else {
                // Save layout
                String name;
                int result = callSaveLayoutDialog(name, false);

                if (result == 1) {
                    tree.storeLayout(name);
                    std::cout << "Save layout: " << name << newLine;
                    postCommandMessage(0);
                }
            }
        }

        // super secret write to file op
        if (key.getKeyCode() == 'x' || key.getKeyCode() == 'X') {
            if (selected.getItemArray().size() == 1) {
                auto item = selected.getSelectedItem(0);
                if (auto effect = dynamic_cast<Effect*>(item.get())) {
                    // Export effect
                    auto effectData = tree.storeEffect(tree.getTree(effect));
                    EffectLoader::writeToFile(effectData);
                }
            } else {
                // Export Layout
                auto layoutData = EffectLoader::loadLayout(tree.getCurrentLayoutName());
                EffectLoader::writeToFile(layoutData);
            }
        }
        // super secret load from file op
        if (key.getKeyCode() == 'o' || key.getKeyCode() == 'O') {
            auto loadData = EffectLoader::loadFromFile();
            std::cout << "load data type: " << loadData.getType().toString() << newLine;
            jassert(loadData.hasProperty("name"));
            if (loadData.getType() == Identifier(EFFECTSCENE_ID)) {
                // Import layout
                EffectLoader::saveLayout(loadData);
                String layoutName = loadData.getProperty("name");
                loadNewLayout(layoutName);
            } else if (loadData.getType() == Identifier(EFFECT_ID)) {
                // Import effect
                EffectLoader::saveEffect(loadData);
                String effectName = loadData.getProperty("name");
                tree.loadEffect(loadData);
            }
        }
        // reset initial use command
        if (key.getKeyCode() == 'I' && key.getModifiers().isCtrlDown())
        {
            tree.clear();

            // If shift is down also erase all saved content
            if (key.getModifiers().isShiftDown()) {
                for (auto l : EffectLoader::getLayoutsAvailable()) {
                    EffectLoader::clearLayout(l);
                }
                for (auto e : EffectLoader::getEffectsAvailable()) {
                    EffectLoader::clearEffect(e);
                }
            }

            getAppProperties().getUserSettings()->clear();
            getAppProperties().getUserSettings()->save();
        }
    }
    
    return true;
}


void EffectScene::storeState() {
    //auto name = tree.getCurrentLayoutName();
    tree.storeLayout();

    getAppProperties().getUserSettings()->setValue(KEYNAME_CURRENT_LOADOUT, "default");
}

/*void EffectScene::updateChannels() {
    auto defaultInChannel = AudioChannelSet();
    defaultInChannel.addChannel(AudioChannelSet::ChannelType::left);
    defaultInChannel.addChannel(AudioChannelSet::ChannelType::right);
    auto defaultOutChannel = AudioChannelSet();
    defaultOutChannel.addChannel(AudioChannelSet::ChannelType::left);
    defaultOutChannel.addChannel(AudioChannelSet::ChannelType::right);

    // Update num ins and outs
    for (auto node : audioGraph.getNodes()) {
        //todo dgaf about buses layout. Only channels within buses.
        // How about a follow-through method that changes channels if possible given port?

        // Set buses layout or whatever
        auto layout = node->getProcessor()->getBusesLayout();

        auto inputs = deviceManager.getAudioDeviceSetup().inputChannels;
        auto outputs = deviceManager.getAudioDeviceSetup().outputChannels;

        for (int i = 0; i < inputs.getHighestBit(); i++) {
            if (inputs[i] == 1) {
                layout.inputBuses.add(defaultInChannel);
            }
        }

        for (int i = 0; i < outputs.getHighestBit(); i++) {
            if (inputs[i] == 1) {
                layout.outputBuses.add(defaultOutChannel);
            }
        }
        node->getProcessor()->setBusesLayout(layout);

        // Tell gui to update
        Effect::updateEffectProcessor(node->getProcessor(), tree);
    }
}*/

void EffectScene::handleCommandMessage(int commandId) {
    if (commandId == 0) {
        getParentComponent()->postCommandMessage(0);
    }
    EffectTreeBase::handleCommandMessage(commandId);
}

bool EffectScene::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    return (dynamic_cast<const Effect*>(other) != nullptr);
}

bool EffectScene::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
}

void EffectScene::menuCreateEffect(ValueTree effectData) {
    effectData.setProperty("x", 400, nullptr);
    effectData.setProperty("y", 400, nullptr);
    tree.loadEffect(effectData);
}

void EffectScene::loadNewLayout(String layout) {
    if (tree.isNotEmpty()) {
        String layoutToSaveName;
        int result = callSaveLayoutDialog(layoutToSaveName, true);

        if (result == -1) {
            return;
        } else if (result == 0) {
            tree.clear();
        } if (result == 1) {
            std::cout << "Save layout: " << layoutToSaveName << newLine;

            tree.storeLayout(layoutToSaveName);
        }
    }

    std::cout << "Load layout: " << layout << newLine;
    tree.loadLayout(layout);
}

int EffectScene::callSaveLayoutDialog(String &name, bool dontSaveButton) {
    AlertWindow saveDialog("Save current layout?", "Enter Layout Name", AlertWindow::AlertIconType::NoIcon);

    saveDialog.addButton("Save", 1, KeyPress(KeyPress::returnKey));
    if (dontSaveButton) {
        saveDialog.addButton("Don't Save", 0);
    }
    saveDialog.addButton("Cancel", -1, KeyPress(KeyPress::escapeKey));

    String currentName = tree.getCurrentLayoutName();
    saveDialog.addTextEditor("Layout Name", currentName);

    auto nameEditor = saveDialog.getTextEditor("Layout Name");

    auto result = saveDialog.runModalLoop();

    name = nameEditor->getText();

    return result;
}

int EffectScene::callSaveEffectDialog(String &name) {
    AlertWindow saveDialog("Save this effect?", "Enter Effect Name", AlertWindow::AlertIconType::NoIcon);

    saveDialog.addButton("Save", 1, KeyPress(KeyPress::returnKey));
    saveDialog.addButton("Cancel", -1, KeyPress(KeyPress::escapeKey));

    saveDialog.addTextEditor("Effect Name", name);

    auto nameEditor = saveDialog.getTextEditor("Effect Name");

    auto result = saveDialog.runModalLoop();

    name = nameEditor->getText();

    return result;
}

StringArray EffectScene::getProcessorNames() {
    return tree.getProcessorNames();
}






