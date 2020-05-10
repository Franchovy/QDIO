#include "EffectScene.h"

const Identifier EffectScene::IDs::DeviceManager = "deviceManager";
EffectScene* EffectScene::instance = nullptr;

//==============================================================================
EffectScene::EffectScene()
        : EffectTreeBase()
        , MenuItem(1)
        , updater(this)
{
    setComponentID("MainWindow");
    setName("MainWindow");

    setBufferedToImage(true);
    //setPaintingIsUnclipped(false);
    setRepaintsOnMouseActivity(false);

    // Set up static members
    instance = this;

    EffectTreeBase::audioGraph = &audioGraph;
    EffectTreeBase::processorPlayer = &processorPlayer;
    EffectTreeBase::deviceManager = &deviceManager;

    audioGraph.enableAllBuses();

    deviceManager.initialise(256, 256, getAppProperties().getUserSettings()->getXmlValue(KEYNAME_DEVICE_SETTINGS).get(),
                             true);

    deviceManager.addAudioCallback(&processorPlayer);
    processorPlayer.setProcessor(&audioGraph);

    setMouseClickGrabsKeyboardFocus(true);

    // Drag Line GUI
    addChildComponent(lasso);

    // Manage EffectsTree


#define BACKGROUND_IMAGE
    bg = ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);
    //bg = ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);
    //bgTile = ImageCache::getFromMemory(BinaryData::bgtile_png, BinaryData::bgtile_pngSize);
    //logo = ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);

    //========================================================================================
    // MIDI example code

    //auto inputDevice  = MidiInput::getDevices()  [MidiInput::getDefaultDeviceIndex()];
    //auto outputDevice = MidiOutput::getDevices() [MidiOutput::getDefaultDeviceIndex()];

    //deviceManager.setMidiInputEnabled (inputDevice, true);
    //deviceManager.addMidiInputCallback (inputDevice, &processorPlayer); // [3]
    //deviceManager.setDefaultMidiOutput (outputDevice);


    //==============================================================================
    // Main component popup menu

    setupCreateEffectMenu();

    /*createGroupEffectItem = PopupMenu::Item("Create Effect with group");
    createGroupEffectItem.setAction([=] {
        createGroupEffect();
    });*/
    //mainMenu.addSubMenu("Create Effect", createEffectMenu);

    appState = loading;
    updater.loadUserState();
    appState = neutral;
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
                auto effectVT = updater.newEffect("Effect", getMouseXYRelative());
                updater.loadEffect(effectVT);
            }));
    createEffectMenu->addItem("Input Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Input Effect");
                auto effectVT = updater.newEffect("Input Device", getMouseXYRelative(), 0);
                updater.loadEffect(effectVT);
            }));
    createEffectMenu->addItem("Output Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Output Effect");
                auto effectVT = updater.newEffect("Output Device", getMouseXYRelative(), 1);
                updater.loadEffect(effectVT);
            }));
    createEffectMenu->addItem("Distortion Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Distortion Effect");
                auto effectVT = updater.newEffect("Distortion Effect", getMouseXYRelative(), 2);
                updater.loadEffect(effectVT);
            }
    ));
    createEffectMenu->addItem("Delay Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Delay Effect");
                auto effectVT = updater.newEffect("Delay Effect", getMouseXYRelative(), 3);
                updater.loadEffect(effectVT);
            }
    ));
    createEffectMenu->addItem("Reverb Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Reverb Effect");
                auto effectVT = updater.newEffect("Reverb Effect", getMouseXYRelative(), 4);
                updater.loadEffect(effectVT);
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
    if (dynamic_cast<Resizer*>(event.originalComponent)) {
        undoManager.beginNewTransaction("resize");
        return;
    }

    if (auto effect = dynamic_cast<Effect*>(event.originalComponent)) {
        if (event.mods.isLeftButtonDown()) {
            //todo static lasso functionality?

            // Drag
            effect->setAlwaysOnTop(true);
            effect->dragger.startDraggingComponent(effect, event);
        } else if (event.mods.isRightButtonDown()) {
            // Send info upwards for menu
            //TODO don't do this, call custom menu function
            getParentComponent()->mouseDown(event);
        }
    }

    std::cout << "Begin new transaction" << newLine;
    undoManager.beginNewTransaction(getName());

    if (auto p = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        dragLine.startDrag(p, event);
    }
}

void EffectScene::mouseDrag(const MouseEvent &event) {
    if (auto effect = dynamic_cast<Effect*>(event.originalComponent)) {
        //todo dragger belonging to effectScene
        effect->dragger.dragComponent(effect, event, nullptr);
        // Manual constraint
        auto newX = jlimit<int>(0, effect->getParentWidth() - effect->getWidth(), effect->getX());
        auto newY = jlimit<int>(0, effect->getParentHeight() - effect->getHeight(), effect->getY());

        effect->setTopLeftPosition(newX, newY);
    }

    /*if (lasso.isVisible())
        lasso.dragLasso(event);*/
    if (event.mods.isLeftButtonDown()) {
        if (dynamic_cast<Effect*>(event.originalComponent)) {
            // Effect drag
            /*
            if (auto newParent = effectToMoveTo(event, tree.getParent())) {
                std::cout << "new parent: " << newParent->getName() << newLine;
                if (newParent != getFromTree<Effect>(tree.getParent())) {
                    auto parent = tree.getParent();

                    parent.removeChild(tree, &undoManager);
                    newParent->getTree().appendChild(tree, &undoManager);

                    if (newParent != this) {
                        SelectHoverObject::setHoverComponent(newParent);
                    } else {
                        SelectHoverObject::resetHoverObject();
                    }
                }
            }*/
        } else if (dynamic_cast<ConnectionPort*>(event.originalComponent)) {
            // Line Drag
            if (lasso.isVisible())
                lasso.dragLasso(event);

            if (event.mods.isLeftButtonDown()) {
                // Line drag
                if (dynamic_cast<ConnectionPort*>(event.originalComponent)) {
                    //TODO efficiency
                    auto port = nullptr;// portToConnectTo(event, tree);

                    if (port != nullptr) {
                        SelectHoverObject::setHoverComponent(port);
                    } else {
                        SelectHoverObject::resetHoverObject();
                    }
                    dragLine.drag(event);
                }
            }
        }
    }
}

void EffectScene::mouseUp(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.endLasso();

    if (event.originalComponent == this) {
        // Click on effectscene
        if (event.getDistanceFromDragStart() < 10)
        {
            if (event.mods.isLeftButtonDown()) {
                // Remove selection
                // Don't deselect if shift or ctrl is held.
                if (! event.mods.isCtrlDown()
                        && ! event.mods.isShiftDown())
                {
                    selected.deselectAll();
                }
            } else if (event.mods.isRightButtonDown()
                    || event.mods.isCtrlDown())
            {
                callMenu(0);
            }
        }
    } else if (auto effect = dynamic_cast<Effect*>(event.originalComponent)) {
        setAlwaysOnTop(false);

        if (event.getDistanceFromDragStart() < 10) {
            if (event.mods.isLeftButtonDown() && !event.mods.isCtrlDown()) {
                addSelectObject(this);
            }
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
    } else if (dynamic_cast<ParameterPort *>(event.originalComponent)) {
        auto port1 = dynamic_cast<ParameterPort *>(dragLine.getPort1());
        auto port2 = dynamic_cast<ParameterPort *>(hoverComponent.get());

        if (port1 != nullptr && port2 != nullptr) {

            // InPort belongs to Internal parameter, but is child of parent effect.
            auto inPort = port1->isInput ? port2 : port1;
            auto outPort = port1->isInput ? port1 : port2;


            // Connect internal Parameter2 to external Parameter1
            auto e = dynamic_cast<Effect *>(outPort->getParentComponent());
            // Internal port belongs to the external parameter & vice versa
            auto inParameter = e->getParameterForPort(outPort);

            auto outParameter = dynamic_cast<Parameter *>(inPort->getParentComponent());

            if (inParameter == nullptr || outParameter == nullptr) {
                std::cout << "Failed to connect parameters!" << newLine;
                dragLine.release(nullptr);
                return;
            }

            // Connect
            outParameter->connect(inParameter);

            auto newConnection = new ConnectionLine(*port1, *port2);
            port1->getDragLineParent()->addAndMakeVisible(newConnection);
        } else {
            dragLine.release(nullptr);
        }
    } else {
        // Call ConnectionPort connect
        if (dynamic_cast<ConnectionPort *>(event.originalComponent)) {
            if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent.get())) {
                // Call create on common parent
                updater.newConnection(port, dragLine.getPort1());
            }
            dragLine.release(nullptr);
        }
    }
}

bool EffectScene::keyPressed(const KeyPress &key)
{

#ifdef DEBUG_UTILITIES
    if (key.getKeyCode() == 's') {
        std::cout << "Audiograph status: " << newLine;
        for (auto node : audioGraph->getNodes()) {
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
    if (key.getKeyCode() == 'e') {
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
    }
    if (key.getKeyCode() == 'l') {
        std::cout << "Available effects: " << newLine;
        for (auto e : EffectLoader::getEffectsAvailable()) {
            std::cout << e << newLine;
        }
    }
    if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'p') {
        std::cout << "Clear effects" << newLine;
        for (auto e : EffectLoader::getEffectsAvailable()) {
            EffectLoader::clearEffect(e);
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


        for (const auto& selectedItem : selected.getItemArray()) {
            updater.remove(selectedItem.get());
        }
        selected.deselectAll();
    }

    if (key.getKeyCode() == KeyPress::escapeKey)
    {
        selected.deselectAll();
    }

    // CTRL
    if (key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown()) {
        if (key.getKeyCode() == 'z') {
            std::cout << "Undo: " << undoManager.getUndoDescription() << newLine;
            undoManager.undo();
        } else if ((key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown())
                   && key.getKeyCode() == 'Z') {
            std::cout << "Redo: " << undoManager.getRedoDescription() << newLine;
            undoManager.redo();
        }

        if (key.getKeyCode() == 's') {
            std::cout << "Save effects" << newLine;
            updater.storeAll();
        }
    }
}


void EffectScene::storeState() {
    updater.storeAll();
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
}


