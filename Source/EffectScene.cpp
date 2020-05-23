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
    // Don't load anything if this isn't the right version.

    bool dontLoad;

    String currentVersion = "1.5";
    std::cout << "App version: " << currentVersion << newLine;
    auto appVersion = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_APP_VERSION);

    auto initialUse = getAppProperties().getUserSettings()->getValue(KEYNAME_INITIAL_USE);
    bool loadInitialCase = true;
    auto initialUseBool = getAppProperties().getUserSettings()->getBoolValue(KEYNAME_INITIAL_USE); //true if prev use
    if (initialUse.isEmpty() && ! initialUseBool) {
        loadInitialCase = true;
    }


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

    //dontLoad = true;
    bool dontLoadDevices = true;

    std::cout << "Loading state? " << (dontLoad ? "false" : "true") << newLine;

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
    processorPlayer.setProcessor(&audioGraph);

    setMouseClickGrabsKeyboardFocus(true);

    // Drag Line GUI
    addChildComponent(lasso);



#define BACKGROUND_IMAGE
    bg = ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);


    // Main component popup menu
    setupCreateEffectMenu();
    
    if (! dontLoad) {
        // Load layout
        appState = loading;
        if (loadInitialCase) {
            // Load initial layout
            auto initialLayout = ValueTree::readFromData(BinaryData::defaultLayout, BinaryData::defaultLayoutSize);
            jassert(initialLayout.getType() == Identifier(EFFECTSCENE_ID));
            EffectLoader::saveLayout(initialLayout);
            auto name = initialLayout.getProperty("name");
            tree.loadLayout(name);
            std::cout << "Initial layout load!" << newLine;

            getAppProperties().getUserSettings()->setValue(KEYNAME_INITIAL_USE, true);
        } else {
            // Load previous layout
            String name = getAppProperties().getUserSettings()->getValue(KEYNAME_CURRENT_LOADOUT);
            std::cout << "Load layout: " << name << newLine;
            tree.loadLayout(name);
        }
        undoManager.clearUndoHistory();
        appState = neutral;
    }

    if (dontLoad) {
        // Clear all settings
        getAppProperties().getUserSettings()->clear();

        // Update load app version
        XmlElement newVersion("version");
        newVersion.setAttribute("version", currentVersion);
        getAppProperties().getUserSettings()->setValue(KEYNAME_APP_VERSION, &newVersion);
        getAppProperties().getUserSettings()->save();
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

void EffectScene::setupCreateEffectMenu() {

    std::unique_ptr<PopupMenu> createEffectMenu = std::make_unique<PopupMenu>();

    createEffectMenu->addItem("Empty Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Effect");
                auto effectVT = tree.newEffect("Effect", getMouseXYRelative());
                tree.createEffect(effectVT);
            }));
    createEffectMenu->addItem("Input Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Input Effect");
                auto effectVT = tree.newEffect("Input Device", getMouseXYRelative(), 0);
                tree.createEffect(effectVT);
            }));
    createEffectMenu->addItem("Output Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Output Effect");
                auto effectVT = tree.newEffect("Output Device", getMouseXYRelative(), 1);
                tree.createEffect(effectVT);
            }));
    createEffectMenu->addItem("Distortion Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Distortion Effect");
                auto effectVT = tree.newEffect("Distortion Effect", getMouseXYRelative(), 2);
                tree.createEffect(effectVT);
            }
    ));
    createEffectMenu->addItem("Delay Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Delay Effect");
                auto effectVT = tree.newEffect("Delay Effect", getMouseXYRelative(), 3);
                tree.createEffect(effectVT);
            }
    ));
    createEffectMenu->addItem("Reverb Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Reverb Effect");
                auto effectVT = tree.newEffect("Reverb Effect", getMouseXYRelative(), 4);
                tree.createEffect(effectVT);
            }
    ));

    PopupMenu::Item createEffectMenuItem("Create effect..");
    createEffectMenuItem.subMenu = std::move(createEffectMenu);

    addMenuItem(0, createEffectMenuItem);
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
            String name;
            int result = callSaveLayoutDialog(name, false);

            if (result == 1) {
                tree.storeLayout(name);
                std::cout << "Save layout: " << name << newLine;
                postCommandMessage(0);
            }
        }

        // super secret write to file op
        if (key.getKeyCode() == 'x' || key.getKeyCode() == 'X') {
            auto layoutData = EffectLoader::loadLayout(tree.getCurrentLayoutName());
            EffectLoader::writeToFile(layoutData);
        }
        // super secret load from file op
        if (key.getKeyCode() == 'o' || key.getKeyCode() == 'O') {
            auto loadData = EffectLoader::loadFromFile();
            std::cout << "load data type: " << loadData.getType().toString() << newLine;
        }
    }
    
    return true;
}


void EffectScene::storeState() {
    auto name = tree.getCurrentLayoutName();
    tree.storeLayout();

    getAppProperties().getUserSettings()->setValue(KEYNAME_CURRENT_LOADOUT, name);
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

bool EffectScene::canDragInto(const SelectHoverObject *other) const {
    return (dynamic_cast<const Effect*>(other) != nullptr);
}

bool EffectScene::canDragHover(const SelectHoverObject *other) const {
    return false;
}

void EffectScene::menuCreateEffect(ValueTree effectData) {
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



