/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

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
    effectsTree.setDragLineTree(dragLine.getDragLineTree()); //This may be making a copy

    //========================================================================================
    // Manage EffectsTree

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
}


MainComponent::~MainComponent()
{
    player.audioDeviceStopped();
    processorGraph.release();
    //TODO add audio device stopper

    /*for (int i = 0; i < effectsTree.getNumChildren(); i++)
        effectsTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject()->decReferenceCount();*/
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    //deviceSelectorGroup.setBounds(10,10,getWidth()-10, getHeight()-10);
    deviceSelector.setBounds(50,50,500,500);
    deviceSelectorComponent.setBounds(50,50,600,600);
}

void MainComponent::mouseDown(const MouseEvent &event) {
    std::cout << "Mouse down on: " << event.originalComponent->getName() << newLine;

    if (event.mods.isRightButtonDown()){

        // Right-click menu
        PopupMenu m;
        menuPos = getMouseXYRelative();

        // TODO custom options depending on event component
        PopupMenu test = getEffectSelectMenu(event);
        m.addSubMenu("Create Effect", test);
        //m.addItem(1, "Create Effect");

        if (deviceSelectorComponent.isVisible())
            m.addItem(2, "Hide Settings");
        else
            m.addItem(2, "Show Settings");
        m.addItem(3, "Run audiograph");
        int result = m.show();

        if (result == 0) {
            // Menu ignored
        } else if (result == -1) {
            // Effect generating code comes from the submenu.
        } else if (result == 2) {
            // Show settings
            deviceSelectorComponent.setVisible(!deviceSelectorComponent.isVisible());
        } else if (result == 3) {
            // Start audio
            initialiseGraph();
        }
    } else if (event.mods.isLeftButtonDown()){
        lasso.setVisible(true);
        lasso.beginLasso(event, this);
    }
    Component::mouseDown(event);
}

void MainComponent::mouseDrag(const MouseEvent &event) {
    lasso.dragLasso(event);
    Component::mouseDrag(event);
}

void MainComponent::mouseUp(const MouseEvent &event) {
    std::cout << "Mouse up" << newLine;
    if (lasso.isVisible())
        lasso.endLasso();

    if (event.mods.isLeftButtonDown()) {
        // Is the component an effect?
        if (auto effect = dynamic_cast<GUIEffect *>(event.originalComponent)) {
            // Scan through effects at this point to see what to do

            std::cout << "Event pos: " << event.getPosition().toString() << newLine;
            std::cout << "Event component: " << event.eventComponent->getPosition().toString() << newLine;
            std::cout << "Event pos relative to this: " << event.getEventRelativeTo(this).getPosition().toString() << newLine;


            auto effects = effectsAt(event.getEventRelativeTo(this).getPosition());
            std::cout << "Effects found: " << effects.size() << newLine;
            for (auto parentEffect : effects) {
                // If there is only this effect
                if (effects.size() == 1 && effects.getFirst() == effect)
                    moveEffect(event.getEventRelativeTo(this), effect->EVT);
                    // If there is another effect
                else if (effect != parentEffect) {
                    std::cout << "Move effect on other: " << parentEffect->getName() << newLine;
                    moveEffect(event.getEventRelativeTo(parentEffect), effect->EVT);
                }
                //
            }
        }
        selected.deselectAll();
    }

    std::cout << "Selected items: " << newLine;
    for (auto i : selected){
        std::cout << i->getName() << newLine;
    }

    Component::mouseUp(event);
}



//==============================================================================

void MainComponent::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    // Add ParameterGroup
    // Add to AudioProcessorGraph
    std::cout << "Added child" << newLine;
    if (childWhichHasBeenAdded.getType() == ID_EFFECT_VT){
        auto effectVT = dynamic_cast<EffectVT*>(childWhichHasBeenAdded.getProperty(ID_EVT_OBJECT).getObject());
        auto effectGui = effectVT->getGUIEffect();
        componentsToSelect.addIfNotAlreadyThere(effectGui);
        //TODO make sure all these are ok!
        if (parentTree.getType() == ID_EFFECT_VT){
            auto parentEffectVT = dynamic_cast<EffectVT*>(parentTree.getProperty(ID_EVT_OBJECT).getObject());
            parentEffectVT->getGUIEffect()->addAndMakeVisible(effectGui);

            auto newPos = effectGui->getPosition() - parentEffectVT->getGUIEffect()->getPosition();
            effectGui->setBounds(newPos.x, newPos.y, effectGui->getWidth(), effectGui->getHeight());

            std::cout << "New position: " << effectGui->getPosition().toString() << newLine;
        } else {
            effectGui->setVisible(true);
            addAndMakeVisible(effectGui);
        }

        for (int i = 0; i < effectVT->getNumChildren(); i++)
            valueTreeChildAdded(effectVT->getTree(), effectVT->getChild(i)->getTree());

        //effectGui->setCentrePosition(menuPos - effectGui->getParentComponent()->getPosition());
    }
}

void MainComponent::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                          int indexFromWhichChildWasRemoved) {
    Listener::valueTreeChildRemoved(parentTree, childWhichHasBeenRemoved, indexFromWhichChildWasRemoved);
}

void MainComponent::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    if (treeWhosePropertyHasChanged == dragLine.getDragLineTree() && property.toString() == "Connection"){
        processorGraph->reset();

        // Add connection
        std::cout << "Add connection!!!!!!!" << newLine;
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
    for (auto i = 0; i < effectsTree.getNumChildren(); i++){
        std::cout << effectsTree.getChild(i)->getName() << newLine;
        auto node = effectsTree.getChild(i)->getNode();
        // add the mothafucka to da graph, dawg

        node->getProcessor()->setPlayConfigDetails(processorGraph->getMainBusNumInputChannels(),
                                                processorGraph->getMainBusNumOutputChannels(),
                                                processorGraph->getSampleRate(),
                                                processorGraph->getBlockSize());
    }

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
                std::cout << "Contains mode" << newLine;
                std::cout << "Area: " << area.toString() << newLine;
                std::cout << "Bounds: " << c->getBoundsInParent().toString() << newLine;
                results.addIfNotAlreadyThere(c);
            }
        } else {
            if (area.intersects(c->getBoundsInParent())) {
                std::cout << "Intersect mode" << newLine;
                std::cout << "Area: " << area.toString() << newLine;
                std::cout << "Bounds: " << c->getBoundsInParent().toString() << newLine;
                results.addIfNotAlreadyThere(c);
            }
        }
    }
}

SelectedItemSet<Component *> &MainComponent::getLassoSelection() {
    selected = SelectedItemSet<Component *>();
    selected.addChangeListener(this);
    return selected;
}

void MainComponent::changeListenerCallback(ChangeBroadcaster *source) {
    std::cout << "Selected Items: " << newLine;
    for (auto c : selected){
        std::cout << c->getName() << newLine;
    }

}

bool MainComponent::keyPressed(const KeyPress &key) {
    return Component::keyPressed(key);
}


/*

void MainComponent::mouseUp(const MouseEvent &event) {
    auto p = getComponentAt(event.getPosition());
    if (p != 0){
        std::cout << "Component: " << p->getName() << newLine;
    } else {
        std::cout << "No result" << newLine;
    }
}
*/



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