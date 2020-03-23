#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() :
        effectsTree("TreeTop"),
        deviceSelector(deviceManager, 0,2,0,2,false,false,true,false),
        processorGraph(new AudioProcessorGraph()),
        deviceSelectorComponent(true),
        dragLine()
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize (1920, 1080);

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

    //player.setProcessor(e);
    //addAndMakeVisible(e->createEditor());
    std::cout << "Test processor: " << player.getCurrentProcessor() << newLine;
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
    addAndMakeVisible(deviceSelectorComponent);

    //==============================================================================
    // Main component popup menu
    mainMenu.addItem("Toggle Settings", [=](){
        deviceSelectorComponent.setVisible(!deviceSelectorComponent.isVisible());
    });

}


MainComponent::~MainComponent()
{

}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    g.fillAll (Colour(30, 35, 40));
}

void MainComponent::resized()
{
    //deviceSelectorGroup.setBounds(10,10,getWidth()-10, getHeight()-10);
    deviceSelector.setBounds(50,50,500,500);
    deviceSelectorComponent.setBounds(50,50,600,600);
}

void MainComponent::mouseDown(const MouseEvent &event) {
    if (event.mods.isRightButtonDown()){
        menuPos = getMouseXYRelative();

        // Right-click menu
        PopupMenu m;

        // Populate menu
        // Add submenus
        PopupMenu test = getEffectSelectMenu(event);
        m.addSubMenu("Create Effect", test);
        // Add CustomMenuItems
        mainMenu.addToMenu(m);
        if (auto e = dynamic_cast<GUIEffect*>(event.originalComponent))
            e->menu.addToMenu(m);
        // Execute result
        int result = m.show();

        if (mainMenu.inRange(result))
            mainMenu.execute(result);
        if (auto e = dynamic_cast<GUIEffect*>(event.originalComponent))
            e->menu.execute(result);

        /*int itemID = 2;

        *//*if (auto e = dynamic_cast<GUIEffect*>(event.originalComponent)) {
            if (!e->isIndividual())
                m.addItem(itemID++, "Toggle Edit Mode");
            if (e->isInEditMode())
                m.addItem(itemID++, "Change effect image...");

        }*//*

        if (auto e = dynamic_cast<GUIEffect*>(event.originalComponent)) {
            e->menu.addToMenu(m);
        }

        if (deviceSelectorComponent.isVisible())
            m.addItem(itemID++, "Hide Settings");
        else
            m.addItem(itemID++, "Show Settings");
        m.addItem(itemID++, "Run audiograph");
        int result = m.show();

        if (result == 0) {
            // Menu ignored
        } else if (result == -1) {
            // Effect generating code comes from the submenu.
        } else if (result == 2 && dynamic_cast<GUIEffect *>(event.originalComponent)) {
            dynamic_cast<GUIEffect *>(event.originalComponent)->toggleEditMode();
        } else if (result == 2) {
            // Show settings
            deviceSelectorComponent.setVisible(!deviceSelectorComponent.isVisible());
        } else if (result == 3) {
            // Start audio
            initialiseGraph();
        }*/
    } else if (event.mods.isLeftButtonDown() && event.originalComponent == this){
        lasso.setVisible(true);
        lasso.beginLasso(event, this);
    }
    //Component::mouseDown(event);
}

void MainComponent::mouseDrag(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.dragLasso(event);
    if (event.mods.isLeftButtonDown()) {
        if (dynamic_cast<LineComponent*>(event.eventComponent)) {

            // Get port to connect to if there is (passing original port parent as componentToIgnore
            auto connectPort = portToConnectTo(event.originalComponent->getParentComponent(),
                    event.getPosition(), effectsTree);

            if (connectPort != nullptr) //TODO
                setHoverComponent(connectPort);
            else
                setHoverComponent(nullptr);

        } else if (auto *effect = dynamic_cast<GUIEffect *>(event.eventComponent)){
            auto newParent = effectToMoveTo(effect,
                                            event.getEventRelativeTo(this).getPosition(), effectsTree);

            if (newParent != this)
                setHoverComponent(newParent);
            else
                setHoverComponent(nullptr);
        }

    }
}

void MainComponent::mouseUp(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.endLasso();

    if (event.mods.isLeftButtonDown()) {
        // Is the component an effect?
        if (GUIEffect* effect = dynamic_cast<GUIEffect *>(event.originalComponent)) {

            if (event.getDistanceFromDragStart() < 10) {
                // Consider this a click and not a drag
                selected.addToSelection(event.eventComponent);
                event.eventComponent->repaint();
            }

            // Scan effect to apply move to
            auto newParent = effectToMoveTo(effect,
                                            event.getEventRelativeTo(this).getPosition(), effectsTree);

            if (auto e = dynamic_cast<GUIEffect*>(newParent))
                if (!e->isInEditMode())
                    return;
            // target is not in edit mode
            if (effect->getParentComponent() != newParent) {
                addEffect(event.getEventRelativeTo(newParent), effect->EVT);
            }
        }
    }

    for (auto i : selected){
        std::cout << i->getName() << newLine;
    }

    Component::mouseUp(event);
}

void MainComponent::mouseEnter(const MouseEvent &event) {
    Component::mouseEnter(event);
}

void MainComponent::mouseExit(const MouseEvent &event) {
    Component::mouseExit(event);
}

//==============================================================================

void MainComponent::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
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

void MainComponent::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                          int indexFromWhichChildWasRemoved) {
    if (childWhichHasBeenRemoved.hasType(ID_EFFECT_VT)){
        // Remove this from its parent
        std::cout << "Remove from parent listener call" << newLine;
    }
}

void MainComponent::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    if (treeWhosePropertyHasChanged == dragLine.getDragLineTree() && property.toString() == "Connection"){
        processorGraph->reset();

        // Add connection
        auto line = dynamic_cast<ConnectionLine*>(treeWhosePropertyHasChanged.getPropertyPointer(property)->getObject());
        addAndMakeVisible(line);
        line->setInterceptsMouseClicks(0,0);
        auto outputPort = line->inPort;
        auto inputPort = line->outPort;

        // Remember that an inputPort is receiving, on the output effect
        // and the outputPort is source on the input effect
        auto output = dynamic_cast<GUIEffect*>(outputPort->getParentComponent())->EVT;
        auto input = dynamic_cast<GUIEffect*>(inputPort->getParentComponent())->EVT;

        //TODO Instead, connect EffectVTs and let connectAudioNodes() do the AudioGraph stuff
        // Add audiograph connection
        for (int c = 0; c < jmin(inputPort->bus->getNumberOfChannels(), outputPort->bus->getNumberOfChannels()); c++) {
            if (processorGraph->addConnection(
                    {{input->getNode()->nodeID, inputPort->bus->getChannelIndexInProcessBlockBuffer(c)},
                     {output->getNode()->nodeID, outputPort->bus->getChannelIndexInProcessBlockBuffer(c)}}))
                std::cout << "Successful connection" << newLine;
            else
                std::cout << "Unsuccessful connection" << newLine;
        }
    }

    Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}

//==============================================================================


void MainComponent::initialiseGraph()
{
    /*for (auto i = 0; i < effectsTree.getNumChildren(); i++){
        std::cout << effectsTree.getChild(i)->getName() << newLine;
        auto node = effectsTree.getChild(i)->getNode();
        // add the mothafucka to da graph, dawg

        node->getProcessor()->setPlayConfigDetails(processorGraph->getMainBusNumInputChannels(),
                                                processorGraph->getMainBusNumOutputChannels(),
                                                processorGraph->getSampleRate(),
                                                processorGraph->getBlockSize());
    }*/

    connectAudioNodes();
}

void MainComponent::connectAudioNodes()
{
    // TODO Check/Compiler algorithm
    if (effectsTree.getNumChildren() == 0){

        return;
    } else {

    }
}

void MainComponent::updateGraph() {
    // What's the difference between here and connectAudioNodes?
    // TODO refresh graph (?)
}


//============================================================================================
// LassoSelector classes

void MainComponent::findLassoItemsInArea(Array<Component *> &results, const Rectangle<int> &area) {
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

SelectedItemSet<Component *> &MainComponent::getLassoSelection() {
    selected = ComponentSelection();
    //selected.addChangeListener(this);

    return selected;
}

bool MainComponent::keyPressed(const KeyPress &key) {
    return Component::keyPressed(key);
}

Component* MainComponent::effectToMoveTo(Component* componentToIgnore, Point<int> point, ValueTree effectTree) {
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

EffectVT::Ptr MainComponent::createEffect(const MouseEvent &event, AudioProcessorGraph::Node::Ptr node)
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
        for (auto eGui : selected.getItemArray()){
            effectVTArray.add(dynamic_cast<GUIEffect*>(eGui)->EVT);
        }
        selected.deselectAll();
        return new EffectVT(event, effectVTArray);
    }
}

Component *MainComponent::portToConnectTo(Component *componentToIgnore, Point<int> point, ValueTree effectTree) {
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e_gui = dynamic_cast<GUIEffect*>(effectTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());

        if (e_gui != nullptr
            && e_gui->getBoundsInParent().contains(point)
            && e_gui != componentToIgnore)
        {
            auto relativePos = point - e_gui->getPosition();

            // Check if there's a match in the children (sending child component coordinates)
            if (auto p = portToConnectTo(componentToIgnore,
                                        relativePos, effectTree.getChild(i)))
                // e != nullptr then the result is returned - corresponding to match in child effect.
                return p;
            else if (auto p = e_gui->checkPort(relativePos))
                // Returns the match if found.
                return p;
        }
    }

    // If nothing is found return nullptr
    return nullptr;
}

/*
void MainComponent::setMidiInput(int index)
{
    auto list = MidiInput::getDevices();

    deviceManager.removeMidiInputCallback (list[lastInputIndex], synthAudioSource.getMidiCollector()); // [13]

    auto newInput = list[index];

    if (! deviceManager.isMidiInputEnabled (newInput))
        deviceManager.setMidiInputEnabled (newInput, true);

    deviceManager.addMidiInputCallback (newInput, synthAudioSource.getMidiCollector()); // [12]
    midiInputList.setSelectedId (index + 1, dontSendNotification);

    lastInputIndex = index;

}
*/
void ComponentSelection::itemSelected(Component *c) {
    if (auto e = dynamic_cast<GUIEffect*>(c))
        e->selectMode = true;
    c->repaint();
}

void ComponentSelection::itemDeselected(Component *c) {
    if (auto e = dynamic_cast<GUIEffect*>(c))
        e->selectMode = false;
    c->repaint();
}
