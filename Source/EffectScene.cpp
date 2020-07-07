#include "EffectScene.h"

const Identifier EffectScene::IDs::DeviceManager = "deviceManager";
EffectScene* EffectScene::instance = nullptr;

//==============================================================================
EffectScene::EffectScene()
        : EffectTreeBase()
        , MenuItem(1)
        , tree(this)
        , connectionGraph(audioGraph)
{
    setComponentID("EffectScene");
    setName("EffectScene");

    //================================================================================
    // LOADING PARAMETERS

    // If this is the Initial Use Case
    loadInitialCase = getAppProperties().getUserSettings()->getBoolValue(KEYNAME_INITIAL_USE, true);


    // Don't load anything if this isn't the right version.

    String currentVersion = "1.7";
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


    //dontLoad = true;
    //bool dontLoadDevices = false;

    loadInitialCase = dontLoad;
    bool dontLoadDevices = dontLoad;

    std::cout << "Loading state? " << (dontLoad ? "false" : "true") << newLine;

    //=======================================================================
    //

    setBufferedToImage(true);
    //setPaintingIsUnclipped(false);
    setRepaintsOnMouseActivity(false);

    hoverable = false;

    // Set up static members
    instance = this;

    Parameter::updater = &parameterUpdater;
    EffectTreeBase::effectScene = this;

    EffectTreeBase::audioGraph = &audioGraph;
    EffectTreeBase::processorPlayer = &processorPlayer;
    EffectTreeBase::deviceManager = &deviceManager;
    EffectTreeBase::connectionGraph = &connectionGraph;

    audioGraph.enableAllBuses();
   
    //======================
    // Set up devices
    if (loadInitialCase) {
        /*auto inNode = audioGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
        auto outNode = audioGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));
        */
        ComboBox inputDevices;
        ComboBox outputDevices;

        //inputDevices.addItemList(deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(true), 0);
        //outputDevices.addItemList(deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(false), 0);

        /*TextButton testButton("TestButton", "Test Setup");
        testButton.onClick = [=] {
            audioGraph.addConnection({{inNode->nodeID, 0}, {outNode->nodeID, 0}});
            audioGraph.addConnection({{inNode->nodeID, 1}, {outNode->nodeID, 1}});
        };*/

        /*inputDevices.onChange = [=] {
            std::cout << inputDevices.getText();
        };
        *//*    auto newDevice = deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(true)[inputDevices.getSelectedItemIndex()];
             auto setup = deviceManager.getAudioDeviceSetup();
        
             setup.inputDeviceName = newDevice;
            
             deviceManager.setAudioDeviceSetup(setup, true);
        };*//*
        
        outputDevices.onChange = [=] {
            auto newDevice = deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(true)[inputDevices.getSelectedItemIndex()];
            auto setup = deviceManager.getAudioDeviceSetup();

            setup.outputDeviceName = newDevice;

            deviceManager.setAudioDeviceSetup(setup, true);
        };*/

        deviceManager.initialiseWithDefaultDevices(2, 2);
        
        AlertWindow deviceSelectWindow("Select your devices", "Select input and output", AlertWindow::AlertIconType::InfoIcon);
        deviceSelectWindow.addComboBox("Input Device", deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(true));
        deviceSelectWindow.addComboBox("Output Device", deviceManager.getCurrentDeviceTypeObject()->getDeviceNames(false));

        deviceSelectWindow.addButton("Let's Go!", 1);

        deviceSelectWindow.runModalLoop();

        auto newSetup = deviceManager.getAudioDeviceSetup();
        newSetup.inputDeviceName = deviceSelectWindow.getComboBoxComponent("Input Device")->getText();
        newSetup.outputDeviceName = deviceSelectWindow.getComboBoxComponent("Output Device")->getText();

        deviceManager.setAudioDeviceSetup(newSetup, true);
    }
    
    
    

        //getAppProperties().getUserSettings()->getXmlValue(KEYNAME_DEVICE_SETTINGS).get()

    if (! dontLoadDevices && getAppProperties().getUserSettings()->getXmlValue((KEYNAME_DEVICE_SETTINGS)) != nullptr) {
        std::cout << "Device state: " << getAppProperties().getUserSettings()->getXmlValue((KEYNAME_DEVICE_SETTINGS))->toString() << newLine;

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


    PopupMenu::Item templateSubMenu("Template..");
    templateSubMenu.subMenu = std::make_unique<PopupMenu>();

    auto saveTemplateItem = PopupMenu::Item("Save to Library..");
    saveTemplateItem.action = [=] { saveTemplate(); };
    templateSubMenu.subMenu->addItem(saveTemplateItem);
    auto exportTemplateItem = PopupMenu::Item("Export..");
    exportTemplateItem.action = [=] { exportTemplate(); };
    templateSubMenu.subMenu->addItem(exportTemplateItem);
    auto importTemplateItem = PopupMenu::Item("Load from file..");
    importTemplateItem.action = [=] { importTemplate(); };
    templateSubMenu.subMenu->addItem(importTemplateItem);
    addMenuItem(0, templateSubMenu);

    PopupMenu::Item loadEffectItem("Load Effect from file..");
    loadEffectItem.action = [=] { importEffect(); };
    addMenuItem(0, loadEffectItem);

    PopupMenu::Item refreshBufferItem("Refresh buffer");
    refreshBufferItem.action = [=] { refreshBuffer(); };
    addMenuItem(0, refreshBufferItem);


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

    Thread::sleep(1000);

    appState = loading;
    startTimer(500);
}

void EffectScene::timerCallback() {
    if (appState == loading) {
        stopTimer();


        // Load
        if (loadInitialCase) {
            /*auto defaultInOut = ValueTree::readFromData(BinaryData::Basic_InOut, BinaryData::Basic_InOutSize);
            EffectLoader::saveTemplate(defaultInOut);
            String name = defaultInOut.getProperty("name");
            loadNewTemplate(name);*/
            // Refresh effect and template menus
            postCommandMessage(0);
        } else if (!dontLoad) {
            // Load template

            // Load previously loaded template
            String name = getAppProperties().getUserSettings()->getValue(KEYNAME_CURRENT_LOADOUT);
            std::cout << "Load template: " << name << newLine;
            auto loadSuccessful = tree.loadTemplate(name);

            if (! loadSuccessful) {
                tree.clear();
            }
            
            undoManager.clearUndoHistory();

        }

        changeListenerCallback(&deviceManager);

        appState = neutral;
    } else {
        // Save app state regularly
        //std::cout << "autosave" << newLine;
        storeState();
    }

    startTimer(15000);
}


void EffectScene::changeListenerCallback(ChangeBroadcaster *source) {
    if (source == &deviceManager) {
        // Update num channels in connections
        if (deviceManager.getCurrentAudioDevice() != nullptr) {

            auto inputChannels = deviceManager.getCurrentAudioDevice()->getActiveInputChannels();
            auto outputChannels = deviceManager.getCurrentAudioDevice()->getActiveOutputChannels();

            int minChannels = jmax(2,
                                   jmin(inputChannels.countNumberOfSetBits(), outputChannels.countNumberOfSetBits()));
            connectionGraph.updateNumChannels(minChannels);

            // DeviceManager change
            if (appState != loading) {
                auto deviceData = deviceManager.createStateXml();

                getAppProperties().getUserSettings()->setValue(KEYNAME_DEVICE_SETTINGS, deviceData.get());
                getAppProperties().getUserSettings()->save();

            }
        }
    }
}


EffectScene::~EffectScene()
{
    appState = stopping;
    
    instance = nullptr;
    //todo remove this ptr from EffectTree before destructor.

    /*audioGraph.clear();
    jassert(! audioGraph.getParameters().isEmpty());*/

    processorPlayer.setProcessor(nullptr);

    //std::cout << "Closing with device settings: " << getAppProperties().getUserSettings()->getXmlValue(KEYNAME_DEVICE_SETTINGS)->toString() << newLine;

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

    if (key.getKeyCode() == 'p') {
        std::cout << "Main Position: " << getMouseXYRelative().toString() << newLine;
        std::cout << "Relative Position: " << getComponentAt(getMouseXYRelative())->getLocalPoint(this, getMouseXYRelative()).toString() << newLine;
    }
    if (key.getKeyCode() == 's') {
        /*std::cout << "Audiograph Nodes: " << newLine;
        for (auto node : audioGraph.getNodes()) {
            std::cout << "node: " << node->nodeID.uid << newLine;
            std::cout << node->getProcessor()->getName() << newLine;
        }*/
        std::cout << "Connections: " << newLine;
        for (auto connection : audioGraph.getConnections()) {
            auto inputNode = audioGraph.getNodeForId(connection.source.nodeID);
            auto outputNode = audioGraph.getNodeForId(connection.destination.nodeID);
            std::cout << "Connection: " << inputNode->getProcessor()->getName() << " to " << outputNode->getProcessor()->getName() << newLine;
        }
    }
    if (key.getKeyCode() == 'r') {
        refreshBuffer();
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
    if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'u') {
        undoManager.clearUndoHistory();
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
                saveEffect();
            } else {
                saveTemplate();
            }
        }

        // super secret write to file op
        if (key.getKeyCode() == 'x' || key.getKeyCode() == 'X') {
            if (selected.getItemArray().size() == 1) {
                auto item = selected.getSelectedItem(0);
                if (auto effect = dynamic_cast<Effect*>(item.get())) {
                    // Export effect
                    exportEffect();
                }
            } else {
                // Export Template
                exportTemplate();
            }
        }
        // super secret load from file op
        if (key.getKeyCode() == 'o' || key.getKeyCode() == 'O') {
            auto loadData = EffectLoader::loadFromFile();
            std::cout << "load data type: " << loadData.getType().toString() << newLine;

            auto numChildren = loadData.getNumChildren();
            if (loadData.getType() == Identifier("data")) {
                for (int i = 0; i < numChildren; i++) {
                    auto child = loadData.getChild(0);
                    loadData.removeChild(child, nullptr);

                    // Basic check for file validity
                    if (! child.hasProperty("name")) {
                        continue;
                    }

                    if (child.getType() == Identifier(EFFECTSCENE_ID)) {
                        importTemplate();
                    } else if (child.getType() == Identifier(EFFECT_ID)) {
                        importEffect();
                    }
                }
            }
        }
        // load new template
        if (key.getKeyCode() == 'n')
        {
            loadNewTemplate("");
        }

        // Select All
        if (key.getKeyCode() == 'a')
        {
            for (auto c : getChildren())
            {
                if (auto obj = dynamic_cast<SelectHoverObject*>(c))
                {
                    selected.addToSelection(obj);
                }
            }
            repaint();
        }

        // Copy
        if (key.getKeyCode() == 'c')
        {
            if (selected.getItemArray().size() == 1) {
                auto data = tree.storeEffect(tree.getTree(selected.getItemArray().getFirst().get()));
                SystemClipboard::copyTextToClipboard(data.toXmlString());
            }
        }

        // Paste
        if (key.getKeyCode() == 'v')
        {
            auto stringData = SystemClipboard::getTextFromClipboard();

            auto data = ValueTree::fromXml(stringData);

            if (data.hasProperty("name")) {
                tree.loadEffect(data);
            }
        }

        // reset initial use command
        if (key.getKeyCode() == 'I' && key.getModifiers().isCtrlDown())
        {
            tree.clear();

            // If shift is down also erase all saved content
            if (key.getModifiers().isShiftDown()) {
                for (auto l : EffectLoader::getTemplatesAvailable()) {
                    EffectLoader::clearTemplate(l);
                }
                for (auto e : EffectLoader::getEffectsAvailable()) {
                    EffectLoader::clearEffect(e);
                }
            }

            getAppProperties().getUserSettings()->clear();
            getAppProperties().getUserSettings()->save();
        }

        if (key.getKeyCode() == '+' || key.getKeyCode() == '=') {
            auto scaleFactor = Desktop::getInstance().getGlobalScaleFactor();
            scaleFactor += 0.1f;
            Desktop::getInstance().setGlobalScaleFactor(scaleFactor);
            repaint();
            return true;
        } else if (key.getKeyCode() == '-' || key.getKeyCode() == '_') {
            auto scaleFactor = Desktop::getInstance().getGlobalScaleFactor();
            scaleFactor -= 0.1f;
            Desktop::getInstance().setGlobalScaleFactor(scaleFactor);
            repaint();
            return true;
        }
    }
    
    return true;
}


void EffectScene::storeState() {
    String templateName = tree.getCurrentTemplateName();

    String name;
    if (! templateName.startsWith("current:")) {
        name = "current:";
        name.append(templateName, 20);
    } else {
        name = templateName;
    }

    tree.storeTemplate(name);

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
        //todo dgaf about buses template. Only channels within buses.
        // How about a follow-through method that changes channels if possible given port?

        // Set buses template or whatever
        auto template = node->getProcessor()->getBusesTemplate();

        auto inputs = deviceManager.getAudioDeviceSetup().inputChannels;
        auto outputs = deviceManager.getAudioDeviceSetup().outputChannels;

        for (int i = 0; i < inputs.getHighestBit(); i++) {
            if (inputs[i] == 1) {
                template.inputBuses.add(defaultInChannel);
            }
        }

        for (int i = 0; i < outputs.getHighestBit(); i++) {
            if (inputs[i] == 1) {
                template.outputBuses.add(defaultOutChannel);
            }
        }
        node->getProcessor()->setBusesTemplate(template);

        // Tell gui to update
        Effect::updateEffectProcessor(node->getProcessor(), tree);
    }
}*/

void EffectScene::handleCommandMessage(int commandId) {
    if (commandId == 0) {
        getParentComponent()->postCommandMessage(0);
    } else if (commandId == 1) {
        // Save effect
        saveEffect();
    } else if (commandId == 2) {
        // Export effect
        exportEffect();
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

void EffectScene::loadNewTemplate(String newTemplate) {
    if (tree.isNotEmpty()) {
        String templateToSaveName;
        int result = callSaveTemplateDialog(templateToSaveName, true);

        if (result == -1) {
            return;
        } else if (result == 0) {
            tree.clear();
        } if (result == 1) {
            std::cout << "Save template: " << templateToSaveName << newLine;

            tree.storeTemplate(templateToSaveName);
            tree.clear();
        }
    }

    std::cout << "Load template: " << newTemplate << newLine;
    bool success = tree.loadTemplate(newTemplate);
    if (! success) {
        std::cout << "Failure loading template! Reloading fresh" << newLine;
        jassertfalse;
        //tree.clear();
    }
}

int EffectScene::callSaveTemplateDialog(String &name, bool dontSaveButton) {
    AlertWindow saveDialog("Save current template?", "Enter Template Name", AlertWindow::AlertIconType::NoIcon);

    saveDialog.addButton("Save", 1, KeyPress(KeyPress::returnKey));
    if (dontSaveButton) {
        saveDialog.addButton("Don't Save", 0);
    }
    saveDialog.addButton("Cancel", -1, KeyPress(KeyPress::escapeKey));

    String currentName = tree.getCurrentTemplateName();
    if (currentName.startsWith("current:")) {
        currentName = currentName.fromLastOccurrenceOf("current:", false, false);
    }
    saveDialog.addTextEditor("Template Name", currentName);

    auto nameEditor = saveDialog.getTextEditor("Template Name");

    auto result = saveDialog.runModalLoop();

    name = nameEditor->getText();

    if (result == 1) {
        if (EffectLoader::getTemplatesAvailable().contains(name)) {
            result = callConfirmOverwriteDialog(name);
            if (result == 0) {
                return callSaveTemplateDialog(name, dontSaveButton);
            }
        }
    }

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

    if (result == 1) {
        if (EffectLoader::getEffectsAvailable().contains(name)) {
            result = callConfirmOverwriteDialog(name);
            if (result == 0) {
                return callSaveEffectDialog(name);
            }
        }
    }

    return result;
}

int EffectScene::callConfirmOverwriteDialog(String &name) {
    String dialogText("Overwrite ");
    dialogText.append(name, 20);
    dialogText.append("?", 1);
    AlertWindow confirmOverwriteDialog("Confirm", dialogText, AlertWindow::AlertIconType::WarningIcon);

    confirmOverwriteDialog.addButton("Overwrite", 1, KeyPress(KeyPress::returnKey));
    confirmOverwriteDialog.addButton("Cancel", -1, KeyPress(KeyPress::escapeKey));
    confirmOverwriteDialog.addButton("Edit name..", 0, KeyPress(KeyPress::backspaceKey));

    return confirmOverwriteDialog.runModalLoop();
}


StringArray EffectScene::getProcessorNames() {
    return tree.getProcessorNames();
}

void EffectScene::refreshBuffer() {
    auto setup = deviceManager.getAudioDeviceSetup();
    auto bufferSizes = deviceManager.getCurrentAudioDevice()->getAvailableBufferSizes();
    auto bufferElement = bufferSizes.indexOf(setup.bufferSize);
    setup.bufferSize = bufferSizes[bufferElement + 1];
    deviceManager.setAudioDeviceSetup(setup, true);
    setup.bufferSize = bufferSizes[bufferElement];
    deviceManager.setAudioDeviceSetup(setup, true);
}

void EffectScene::importTemplate() {
    auto loadData = EffectLoader::loadFromFile();
    std::cout << "load data type: " << loadData.getType().toString() << newLine;

    auto numChildren = loadData.getNumChildren();
    if (loadData.getType() == Identifier("data")) {
        for (int i = 0; i < numChildren; i++) {
            auto child = loadData.getChild(0);
            loadData.removeChild(child, nullptr);

            // Basic check for file validity
            if (! child.hasProperty("name")) {
                continue;
            }

            if (child.getType() == Identifier(EFFECTSCENE_ID)) {
                // Import template
                EffectLoader::saveTemplate(child);
                String templateName = child.getProperty("name");
                loadNewTemplate(templateName);
            }
        }
    }
}

void EffectScene::exportTemplate() {
    auto templateData = EffectLoader::loadTemplate(tree.getCurrentTemplateName());
    EffectLoader::writeToFile(templateData);
}

void EffectScene::importEffect() {
    auto loadData = EffectLoader::loadFromFile();
    std::cout << "load data type: " << loadData.getType().toString() << newLine;

    auto numChildren = loadData.getNumChildren();
    if (loadData.getType() == Identifier("data")) {
        for (int i = 0; i < numChildren; i++) {
            auto child = loadData.getChild(0);
            loadData.removeChild(child, nullptr);

            // Basic check for file validity
            if (! child.hasProperty("name")) {
                continue;
            }

            if (child.getType() == Identifier(EFFECT_ID)) {
                // Import effect
                EffectLoader::saveEffect(child);
                tree.loadEffect(child);

            }
        }
    }
}

void EffectScene::exportEffect() {
    if (selected.getItemArray().size() == 1) {
        auto item = selected.getSelectedItem(0);
        if (auto effect = dynamic_cast<Effect*>(item.get())) {
            // Export effect
            effect->setEditMode(false);
            auto effectData = tree.storeEffect(tree.getTree(effect));
            EffectLoader::writeToFile(effectData);
        }
    }
}

void EffectScene::saveTemplate() {
    String name;
    int result = callSaveTemplateDialog(name, false);


    if (result == 1) {
        tree.storeTemplate(name);
        std::cout << "Save template: " << name << newLine;
        postCommandMessage(0);
    }
}

void EffectScene::saveEffect() {
    if (selected.getItemArray().size() == 1) {
        auto item = selected.getSelectedItem(0);

        if (auto effect = dynamic_cast<Effect*>(item.get())) {
            String name = item->getName();

            int result = callSaveEffectDialog(name);

            if (result == 1) {
                effect->setEditMode(false);
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
    }
}


