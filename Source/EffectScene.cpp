#include "EffectScene.h"

//==============================================================================
EffectScene::EffectScene() :
        effectsTree("TreeTop"),
        deviceSelector(deviceManager, 0,2,0,2,false,false,true,false),
        processorGraph(new AudioProcessorGraph()),
        deviceSelectorComponent(true),
        dragLine()
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize (4000, 4000);


    //========================================================================================
    // Drag Line GUI
    addChildComponent(dragLine);
    dragLine.setAlwaysOnTop(true);
    addChildComponent(lasso);
    dragLine.getDragLineTree().addListener(this);
    effectsTree.appendChild(dragLine.getDragLineTree(), nullptr);

    //========================================================================================
    // Manage EffectsTree

    effectsTree.setProperty(ID_EFFECT_GUI, this, nullptr);
    //effectsTree.setProperty(ID_EFFECT_GUI, this);
    effectsTree.addListener(this);
    EffectVT::setAudioProcessorGraph(processorGraph.get());

    //========================================================================================
    // Manage Audio

    //auto inputDevice  = MidiInput::getDevices()  [MidiInput::getDefaultDeviceIndex()];
    //auto outputDevice = MidiOutput::getDevices() [MidiOutput::getDefaultDeviceIndex()];

    processorGraph->enableAllBuses();

    auto savedState = getAppProperties().getUserSettings()->getXmlValue (KEYNAME_DEVICE_SETTINGS);
    deviceManager.initialise (256, 256, savedState.get(), true);

    deviceManager.addAudioCallback (&player);                  // [2]

    //deviceManager.setMidiInputEnabled (inputDevice, true);
    //deviceManager.addMidiInputCallback (inputDevice, &player); // [3]
    //deviceManager.setDefaultMidiOutput (outputDevice);

    player.setProcessor (processorGraph.get());

    //==============================================================================
    // DeviceSelector GUI

    deviceSelectorComponent.addAndMakeVisible(deviceSelector);
    deviceSelectorComponent.setTitle("Device Settings");
    deviceSelectorComponent.setSize(deviceSelector.getWidth(), deviceSelector.getHeight()+150);
    deviceSelectorComponent.closeButton.onClick = [=]{
        deviceSelectorComponent.setVisible(false);
        auto audioState = deviceManager.createStateXml();

        getAppProperties().getUserSettings()->setValue (KEYNAME_DEVICE_SETTINGS, audioState.get());
        getAppProperties().getUserSettings()->saveIfNeeded();
    };
    addChildComponent(deviceSelectorComponent);

    //==============================================================================
    // Main component popup menu
    mainMenu = std::make_unique<CustomMenuItems>();

    mainMenu->addItem("Toggle Settings", [=](){
        deviceSelectorComponent.setVisible(!deviceSelectorComponent.isVisible());
    });

}


EffectScene::~EffectScene()
{
    //processorGraph->releaseResources();

    // kinda messy managing one's own ReferenceCountedObject property.
    jassert(getReferenceCount() == 1);
    incReferenceCount();
    effectsTree.removeProperty(ID_EFFECT_GUI, nullptr);
    decReferenceCountWithoutDeleting();

    player.setProcessor(nullptr);
    deviceManager.closeAudioDevice();
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
    if (getParentComponent() != nullptr)
        setBounds(getParentComponent()->getBounds());
    //deviceSelectorGroup.setBounds(10,10,getWidth()-10, getHeight()-10);
    deviceSelector.setBounds(50,50,500,500);
    deviceSelectorComponent.setBounds(50,50,600,600);
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
                treeToCheck = dynamic_cast<GUIEffect*>(parentToCheck)->EVT->getTree();
            else
                treeToCheck = effectsTree;
            connectPort = portToConnectTo(newEvent, treeToCheck);

            if (connectPort != nullptr) {
                setHoverComponent(connectPort);
            } else
                setHoverComponent(nullptr);

        }
        // Effect drag
        else if (auto *effect = dynamic_cast<GUIEffect *>(event.eventComponent)){
            auto newParent = effectToMoveTo(effect,
                                            event.getEventRelativeTo(this).getPosition(), effectsTree);

            if (newParent != this)
                setHoverComponent(dynamic_cast<GuiObject*>(newParent));
            else
                setHoverComponent(nullptr);
        }
    }
}

void EffectScene::mouseUp(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.endLasso();

    if (event.mods.isLeftButtonDown()) {
        // If the component is an effect, respond to move effect event
        if (GUIEffect *effect = dynamic_cast<GUIEffect *>(event.originalComponent)) {
            if (event.getDistanceFromDragStart() < 10) {
                // Consider this a click and not a drag
                selected.addToSelection(dynamic_cast<GuiObject*>(event.eventComponent));
                event.eventComponent->repaint();
            }

            // Scan effect to apply move to
            auto newParent = effectToMoveTo(effect,
                                            event.getEventRelativeTo(this).getPosition(), effectsTree);

            if (auto e = dynamic_cast<GUIEffect *>(newParent))
                if (!e->isInEditMode())
                    return;
            // target is not in edit mode
            if (effect->getParentComponent() != newParent) {
                addEffect(event.getEventRelativeTo(newParent), effect->EVT);
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
            if (auto e = dynamic_cast<GUIEffect*>(event.originalComponent))
                e->getMenu().addToMenu(m);
            // Execute result
            int result = m.show();

            if (mainMenu->inRange(result))
                mainMenu->execute(result);
            if (auto e = dynamic_cast<GUIEffect*>(event.originalComponent))
                e->getMenu().execute(result);
    }

    for (auto i : selected){
        std::cout << i->getName() << newLine;
    }

    Component::mouseUp(event);
}

void EffectScene::mouseEnter(const MouseEvent &event) {
    Component::mouseEnter(event);
}

void EffectScene::mouseExit(const MouseEvent &event) {
    Component::mouseExit(event);
}

bool EffectScene::keyPressed(const KeyPress &key) {
    if (key == KeyPress::deleteKey) {
        if (!selected.getItemArray().isEmpty()) {
            // Delete selected
            for (auto i : selected.getItemArray()) {
                if (auto e = dynamic_cast<GUIEffect*>(i.get()))
                    deleteEffect(e);
            }
        }
    }


    return Component::keyPressed(key);
}

void EffectScene::deleteEffect(GUIEffect* e) {
    delete e;
}

//==============================================================================

void EffectScene::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    if (childWhichHasBeenAdded.getType() == ID_EFFECT_VT){
        auto effectVT = dynamic_cast<EffectVT*>(childWhichHasBeenAdded.getProperty(ID_EVT_OBJECT).getObject());
        auto effectGui = effectVT->getGUIEffect();
        componentsToSelect.addIfNotAlreadyThere(effectGui);

        if (parentTree.getType() == ID_EFFECT_VT){
            auto parentEffectVT = dynamic_cast<EffectVT*>(parentTree.getProperty(ID_EVT_OBJECT).getObject());

            // Add this gui to new parent
            parentEffectVT->getGUIEffect()->addAndMakeVisible(effectGui);
        } else {
            effectGui->setVisible(true);
            addAndMakeVisible(effectGui);
        }

        // Set initialised boolean in GUIEffect
        effectGui->hasBeenInitialised = true;

        //TODO check if recursive call for children is needed
        for (int i = 0; i < effectVT->getNumChildren(); i++)
            valueTreeChildAdded(effectVT->getTree(), effectVT->getChild(i)->getTree());
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
    if (treeWhosePropertyHasChanged == dragLine.getDragLineTree() && property.toString() == "Connection"){
        processorGraph->reset();

        // Add connection
        auto line = dynamic_cast<ConnectionLine*>(treeWhosePropertyHasChanged.getPropertyPointer(property)->getObject());

        auto outputPort = line->inPort;
        auto inputPort = line->outPort;

        // Remember that an inputPort is receiving, on the output effect
        // and the outputPort is source on the input effect
        auto output = dynamic_cast<GUIEffect*>(outputPort->getParent());
        auto input = dynamic_cast<GUIEffect*>(inputPort->getParent());

        // Check for common parent
        // Find common parent
        if (input->getParentComponent() == output->getParentComponent()) {
            std::cout << "Common parent" << newLine;
            if (input->getParentComponent() == this) { //todo u must change dis shit brugh
                connections.add(line);
                addAndMakeVisible(line);
            } else {
                dynamic_cast<GUIEffect*>(input->getParentComponent())->EVT->addConnection(line);
            }
        } else if (input->getParentComponent() == output) {
            std::cout << "Output parent" << newLine;
            if (output->getParentComponent() == this) { //todo u must change dis shit brugh
                connections.add(line);
                addAndMakeVisible(line);
            } else {
                dynamic_cast<GUIEffect*>(output->getParentComponent())->EVT->addConnection(line);
            }
        } else if (output->getParentComponent() == input) {
            std::cout << "Input parent" << newLine;
            if (input->getParentComponent() == this) { //todo u must change dis shit brugh
                connections.add(line);
                addAndMakeVisible(line);
            } else {
                dynamic_cast<GUIEffect*>(input->getParentComponent())->EVT->addConnection(line);
            }
        }

        // Update audiograph
        addAudioConnection(line);

        //timerCallback();

    }

    Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}

//==============================================================================


//============================================================================================
// LassoSelector classes

void EffectScene::findLassoItemsInArea(Array<GuiObject::Ptr> &results, const Rectangle<int> &area) {
    for (auto c : componentsToSelect) {
        if (!intersectMode) {
            if (area.contains(c->getBoundsInParent())) {
                results.addIfNotAlreadyThere(c);
            }
        } else {
            if (area.intersects(c->getBoundsInParent())) {
                results.addIfNotAlreadyThere(c);
            }
        }
    }
}

SelectedItemSet<GuiObject::Ptr> &EffectScene::getLassoSelection() {
    selected.clear();
    //selected.addChangeListener(this);

    return selected;
}

void EffectScene::setHoverComponent(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<GUIEffect*>(hoverComponent.get())) {
        e->hoverMode = false;
        e->repaint();
    } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent.get())) {
        p->hoverMode = false;
        p->repaint();
    }

    hoverComponent = c;

    if (auto e = dynamic_cast<GUIEffect*>(hoverComponent.get())) {
        e->hoverMode = true;
        e->repaint();
    } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent.get())) {
        p->hoverMode = true;
        p->repaint();
    }
};



Component* EffectScene::effectToMoveTo(Component* componentToIgnore, Point<int> point, ValueTree effectTree) {
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e_gui = dynamic_cast<GUIEffect*>(effectTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());

        if (e_gui != nullptr
                && e_gui->getBoundsInParent().contains(point)
                && e_gui != componentToIgnore)
        {
            // Add any filters here
            if (e_gui->isIndividual()){
                continue;
            }

            // Check if there's a match in the children (sending child component coordinates)
            if (auto e = effectToMoveTo(componentToIgnore,
                    point - e_gui->getPosition(), effectTree.getChild(i)))
                return e;
            else
                return e_gui;
        }
    }
    // If nothing is found (at topmost level) then return the maincomponent
    if (effectTree == effectsTree)
        return this;
    else return nullptr;
}

EffectVT::Ptr EffectScene::createEffect(const MouseEvent &event, const AudioProcessorGraph::Node::Ptr& node)
{
    if (node != nullptr){
        // Individual effect from processor
        return new EffectVT(event, node->nodeID);
    }
    if (selected.getNumSelected() == 0){
        // Empty effect
        return new EffectVT(event);
    } else if (selected.getNumSelected() > 0){
        // Create Effect with selected Effects inside
        Array<const EffectVT*> effectVTArray;
        for (auto item : selected.getItemArray()){
            if (auto eGui = dynamic_cast<GUIEffect*>(item.get()))
                effectVTArray.add(eGui->EVT);
        }
        selected.deselectAll();
        return new EffectVT(event, effectVTArray);
    }
}


/**
 * Adds existing effect as child to the effect under the mouse
 * @param event for which the location will determine what effect to add to.
 * @param childEffect effect to add
 */
void EffectScene::addEffect(const MouseEvent& event, EffectVT::Ptr childEffect, bool addToMain) {
    auto parentTree = childEffect->getTree().getParent();
    parentTree.removeChild(childEffect->getTree(), nullptr);

    ValueTree newParent;
    if (auto newGUIEffect = dynamic_cast<GUIEffect*>(event.eventComponent)){
        newParent = newGUIEffect->EVT->getTree();
    } else
        newParent = effectsTree;

    newParent.appendChild(childEffect->getTree(), nullptr);
}

PopupMenu EffectScene::getEffectSelectMenu(const MouseEvent &event) {
    PopupMenu m;
    m.addItem("Empty Effect", std::function<void()>(
            [=]{
                auto e = createEffect(event);
                addEffect(event, e);
            }));
    m.addItem("Input Effect", std::function<void()>(
            [=]{
                auto node = processorGraph->addNode(std::make_unique<InputDeviceEffect>());
                auto e = createEffect(event, node);
                addEffect(event, e);
            }));
    m.addItem("Output Effect", std::function<void()>(
            [=]{
                auto node = processorGraph->addNode(std::make_unique<OutputDeviceEffect>());
                auto e = createEffect(event, node);
                addEffect(event, e);
            }));
    m.addItem("Delay Effect", std::function<void()>(
            [=](){
                auto node = processorGraph->addNode(std::make_unique<DelayEffect>());
                node->getProcessor()->setPlayConfigDetails(
                        processorGraph->getMainBusNumInputChannels(),
                        processorGraph->getMainBusNumOutputChannels(),
                        processorGraph->getSampleRate(),
                        processorGraph->getBlockSize()
                );
                auto e = createEffect(event, node);
                addEffect(event, e);
            }
    ));
    m.addItem("Distortion Effect", std::function<void()>(
            [=]{
                auto node = processorGraph->addNode(std::make_unique<DistortionEffect>());
                node->getProcessor()->setPlayConfigDetails(
                        processorGraph->getMainBusNumInputChannels(),
                        processorGraph->getMainBusNumOutputChannels(),
                        processorGraph->getSampleRate(),
                        processorGraph->getBlockSize()
                );
                auto e = createEffect(event, node);
                addEffect(event, e);
            }
    ));
    return m;
}

ConnectionPort::Ptr EffectScene::portToConnectTo(MouseEvent& event, const ValueTree& effectTree) {

    // Check for self ports
    if (auto p = dynamic_cast<ConnectionPort*>(event.originalComponent))
        if (auto e = dynamic_cast<GUIEffect*>(event.eventComponent))
            if (auto returnPort = e->checkPort(event.getPosition()))
                if (p->canConnect(returnPort))
                    return returnPort;

    // Check children for a match
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e_gui = dynamic_cast<GUIEffect*>(effectTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());

        if (e_gui == nullptr)
            continue;

        // Filter self effect if AudioPort
        if (auto p = dynamic_cast<AudioPort*>(event.originalComponent))
            if (p->getParent() == e_gui)
                continue;

        auto localPos = e_gui->getLocalPoint(event.eventComponent, event.getPosition());

        std::cout << "Local event: " <<  e_gui->getLocalPoint(event.eventComponent, event.getPosition()).toString() << newLine;
        std::cout << "event pos: " << event.getPosition().toString() << newLine;

        if (e_gui->contains(localPos))
        {
            if (auto p = e_gui->checkPort(localPos))
                if (dynamic_cast<ConnectionPort*>(event.originalComponent)->canConnect(p))
                    return p;
        }
    }

    // If nothing is found return nullptr
    return nullptr;
}

void EffectScene::addAudioConnection(ConnectionLine *connectionLine) {
    EffectVT::NodeAndPort in;
    EffectVT::NodeAndPort out;
    auto inEVT = connectionLine->inPort->getParent()->EVT;
    auto outEVT = connectionLine->outPort->getParent()->EVT;

    in = inEVT->getNode(connectionLine->inPort);
    out = outEVT->getNode(connectionLine->outPort);

    if (in.isValid && out.isValid) {
        for (int c = 0; c < jmin(numInputChannels, numOutputChannels); c++) {
            AudioProcessorGraph::Connection connection = {{in.node->nodeID, in.port->bus->getChannelIndexInProcessBlockBuffer(c)},
                                               {out.node->nodeID, out.port->bus->getChannelIndexInProcessBlockBuffer(c)}};
            if (!processorGraph->isConnected(connection) && processorGraph->isConnectionLegal(connection)) {
                processorGraph->addConnection(connection);
            }
        }
    }
}

void EffectScene::mouseMove(const MouseEvent &event) {
}


void ComponentSelection::itemSelected(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<GUIEffect*>(c.get()))
        e->selectMode = true;
    c->repaint();
}

void ComponentSelection::itemDeselected(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<GUIEffect*>(c.get()))
        e->selectMode = false;
    c->repaint();
}
