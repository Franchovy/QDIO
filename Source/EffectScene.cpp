#include "EffectScene.h"


//==============================================================================
EffectScene::EffectScene() :
        EffectTreeBase(ID_TREE_TOP)
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize (4000, 4000);

    //setMouseClickGrabsKeyboardFocus(true);

    //========================================================================================
    // Drag Line GUI
    addChildComponent(dragLine);
    dragLine.addComponentListener(this);
    dragLine.setAlwaysOnTop(true);
    addChildComponent(lasso);

    //========================================================================================
    // Manage EffectsTree

    tree.setProperty(ID_EFFECT_GUI, this, nullptr);
    //tree.setProperty(ID_EFFECT_GUI, this);
    tree.addListener(this);

    //========================================================================================
    // Manage Audio

    //auto inputDevice  = MidiInput::getDevices()  [MidiInput::getDefaultDeviceIndex()];
    //auto outputDevice = MidiOutput::getDevices() [MidiOutput::getDefaultDeviceIndex()];






    //deviceManager.setMidiInputEnabled (inputDevice, true);
    //deviceManager.addMidiInputCallback (inputDevice, &processorPlayer); // [3]
    //deviceManager.setDefaultMidiOutput (outputDevice);


    //==============================================================================
    // DeviceSelector GUI

    //TODO custom Device selector (without GUIWrapper)
    /*deviceSelector(*deviceManager, 0,2,0,2,false,false,true,false);
            deviceSelectorComponent(true);

    deviceSelectorComponent->addAndMakeVisible(deviceSelector);
    deviceSelectorComponent.setTitle("Device Settings");
    deviceSelectorComponent.setSize(deviceSelector.getWidth(), deviceSelector.getHeight()+150);
    deviceSelectorComponent.closeButton.onClick = [=]{
        deviceSelectorComponent.setVisible(false);
        auto audioState = deviceManager->createStateXml();

        getAppProperties().getUserSettings()->setValue (KEYNAME_DEVICE_SETTINGS, audioState.get());
        getAppProperties().getUserSettings()->saveIfNeeded();
    };
    addChildComponent(deviceSelectorComponent);
*/

    //==============================================================================
    // Main component popup menu
    mainMenu = std::make_unique<CustomMenuItems>();

    mainMenu->addItem("Toggle Settings", [=](){
        //deviceSelectorComponent.setVisible(!deviceSelectorComponent.isVisible());
    });
}

EffectScene::~EffectScene()
{
    //audioGraph->releaseResources();

    // kinda messy managing one's own ReferenceCountedObject property.
    jassert(getReferenceCount() == 1);
    incReferenceCount();
    tree.removeProperty(ID_EFFECT_GUI, nullptr);
    decReferenceCountWithoutDeleting();
}

//==============================================================================
void EffectScene::paint (Graphics& g)
{
    g.fillAll (Colour(30, 35, 40));
    
    // bg logo
    g.setOpacity(0.8f);
    g.setColour(Colours::ghostwhite);
    g.drawEllipse(getWidth()/2 - 250, getHeight()/2 - 250, 500, 500, 0.2f);
    
    g.setOpacity(0.3f);
    g.setFont(30);
    
    g.drawText("QDIO",getWidth()/2-50, getHeight()/2-50, 100, 100, Justification::centred);
}

void EffectScene::resized()
{
    setBounds(0, 0, 1920, 1080);
}

void EffectScene::mouseDown(const MouseEvent &event) {
    if (event.mods.isLeftButtonDown() && event.originalComponent == this){
        lasso.setVisible(true);
        lasso.beginLasso(event, this);
    }
    //Component::mouseDown(event);
}

void EffectScene::mouseDrag(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.dragLasso(event);
    if (event.mods.isLeftButtonDown()) {
        // Line drag
        if (dynamic_cast<LineComponent*>(event.eventComponent)) {
            // ParentToCheck is the container of possible things to connect to.
            Component* parentToCheck;
            if (dynamic_cast<AudioPort*>(event.originalComponent))
                parentToCheck = event.originalComponent->getParentComponent()->getParentComponent();
            else if (dynamic_cast<InternalConnectionPort*>(event.originalComponent))
                parentToCheck = event.originalComponent->getParentComponent();

            //auto connectPort = portToConnectTo(newEvent, parentToCheck)
            ConnectionPort::Ptr connectPort;
            // Get port to connect to (if there is one)
            auto newEvent = event.getEventRelativeTo(parentToCheck);

            //TODO EffectScene and EffectTree parent under one subclass
            ValueTree treeToCheck;
            if (parentToCheck != this)
                treeToCheck = dynamic_cast<Effect*>(parentToCheck)->getTree();
            else
                treeToCheck = tree;
            connectPort = portToConnectTo(newEvent, treeToCheck);

            if (connectPort != nullptr) {
                setHoverComponent(connectPort);
            } else
                setHoverComponent(nullptr);

        }
        // Effect drag
        else if (auto effect = dynamic_cast<Effect *>(event.eventComponent)){
            auto newParent = effectToMoveTo(event.getEventRelativeTo(this), tree);

            if (newParent != this)
                setHoverComponent(dynamic_cast<GuiObject*>(newParent));
            else
                setHoverComponent(nullptr);
        }
    }
}

void EffectScene::mouseUp(const MouseEvent &event) {
    // TODO implement all of this locally
    if (lasso.isVisible())
        lasso.endLasso();

    if (event.mods.isLeftButtonDown()) {
        // If the component is an effect, respond to move effect event
        if (auto effect = dynamic_cast<Effect *>(event.originalComponent)) {
            if (event.getDistanceFromDragStart() < 10) {
                // Consider this a click and not a drag
                selected.addToSelection(dynamic_cast<GuiObject*>(event.eventComponent));
                event.eventComponent->repaint();
            }

            // Scan effect to apply move to
            auto newParent = effectToMoveTo(event.getEventRelativeTo(this), tree);

            if (auto e = dynamic_cast<Effect *>(newParent))
                if (!e->isInEditMode())
                    return;
            // target is not in edit mode
            if (effect->getParentComponent() != newParent) {
                addEffect(event.getEventRelativeTo(newParent), *effect);
            }
        }
        // If component is LineComponent, respond to line drag event
        else if (auto l = dynamic_cast<LineComponent *>(event.eventComponent)) {
            if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent.get())) {
                l->convert(port);
            }
        }
    }
        
    // Open menu - either right click or left click (for mac)
    if (event.mods.isRightButtonDown() ||
               (event.getDistanceFromDragStart() < 10 &&
                event.mods.isLeftButtonDown() &&
                event.mods.isCtrlDown())) {

            // Right-click menu
            PopupMenu m;

            // Populate menu
            // Add submenus
            PopupMenu createEffectSubmenu = getEffectSelectMenu(event);
            m.addSubMenu("Create Effect", createEffectSubmenu);
            // Add CustomMenuItems
            mainMenu->addToMenu(m);
            if (auto e = dynamic_cast<Effect*>(event.originalComponent))
                e->getMenu().addToMenu(m);
            // Execute result
            int result = m.show();

            if (mainMenu->inRange(result))
                mainMenu->execute(result);
            if (auto e = dynamic_cast<Effect*>(event.originalComponent))
                e->getMenu().execute(result);
    }

    for (auto i : selected){
        std::cout << i->getName() << newLine;
    }

    Component::mouseUp(event);
}

bool EffectScene::keyPressed(const KeyPress &key)
{
    //std::cout << "Key press: " << key.getKeyCode() << newLine;

    if (key == KeyPress::deleteKey) {
        if (!selected.getItemArray().isEmpty()) {
            // Delete selected
            for (auto i : selected.getItemArray()) {
                if (auto e = dynamic_cast<Effect*>(i.get()))
                    deleteEffect(e);
            }
        }
    }

    if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'z') {
        std::cout << "Undo: " << undoManager.getCurrentTransactionName() << newLine;
        undoManager.undo();
    } else if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'Z') {
        undoManager.redo();
    }

    return Component::keyPressed(key);
}

void EffectScene::deleteEffect(Effect* e) {
    delete e;
}

//==============================================================================

void EffectScene::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    if (childWhichHasBeenAdded.getType() == ID_EFFECT_VT){
        auto effect = dynamic_cast<Effect*>(childWhichHasBeenAdded.getProperty(ID_EVT_OBJECT).getObject());

        //TODO componentsToSelect update
        //componentsToSelect.addIfNotAlreadyThere(effect);

        /*auto parent = dynamic_cast<EffectTreeBase*>(parentTree.getProperty(EffectTreeBase::IDs::effectTreeBase).getObject());
        parent->addEffect(effect);*/
    }
}

void EffectScene::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                        int indexFromWhichChildWasRemoved) {
    if (childWhichHasBeenRemoved.hasType(ID_EFFECT_VT)){
        // Remove this from its parent
        std::cout << "Remove from parent listener call" << newLine;
    }
}

void EffectScene::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {

    Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}

//==============================================================================


//============================================================================================






PopupMenu EffectScene::getEffectSelectMenu(const MouseEvent &event) {
    PopupMenu m;
    m.addItem("Empty Effect", std::function<void()>(
            [=]{
                auto e = createEffect(event);
                addEffect(event, *e);
            }));
    m.addItem("Input Effect", std::function<void()>(
            [=]{
                auto node = audioGraph->addNode(std::make_unique<InputDeviceEffect>());
                auto e = createEffect(event, node);
                addEffect(event, *e);
            }));
    m.addItem("Output Effect", std::function<void()>(
            [=]{
                auto node = audioGraph->addNode(std::make_unique<OutputDeviceEffect>());
                auto e = createEffect(event, node);
                addEffect(event, *e);
            }));
    m.addItem("Delay Effect", std::function<void()>(
            [=](){
                auto node = audioGraph->addNode(std::make_unique<DelayEffect>());
                node->getProcessor()->setPlayConfigDetails(
                        audioGraph->getMainBusNumInputChannels(),
                        audioGraph->getMainBusNumOutputChannels(),
                        audioGraph->getSampleRate(),
                        audioGraph->getBlockSize()
                );
                auto e = createEffect(event, node);
                addEffect(event, *e);
            }
    ));
    m.addItem("Distortion Effect", std::function<void()>(
            [=]{
                auto node = audioGraph->addNode(std::make_unique<DistortionEffect>());
                node->getProcessor()->setPlayConfigDetails(
                        audioGraph->getMainBusNumInputChannels(),
                        audioGraph->getMainBusNumOutputChannels(),
                        audioGraph->getSampleRate(),
                        audioGraph->getBlockSize()
                );
                auto e = createEffect(event, node);
                addEffect(event, *e);
            }
    ));
    return m;
}

void ComponentSelection::itemSelected(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<Effect*>(c.get()))
        e->setSelectMode(true);
    c->repaint();
}

void ComponentSelection::itemDeselected(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<Effect*>(c.get()))
        e->setSelectMode(false);
    c->repaint();
}
