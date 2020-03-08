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
        deviceSelectorComponent()
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize (1920, 1080);


    //========================================================================================
    // Manage EffectsTree

    effectsTree.addListener(this);

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
}

MainComponent::~MainComponent()
{
    //TODO add audio device stopper

    for (int i = 0; i < effectsTree.getNumChildren(); i++)
        effectsTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject()->decReferenceCount();
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setFont (Font (16.0f));
    g.setColour (Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), Justification::centred, true);
}

void MainComponent::resized()
{
    //deviceSelectorGroup.setBounds(10,10,getWidth()-10, getHeight()-10);
    deviceSelectorComponent.setBounds(50,50,getWidth()-50,getHeight()-50);
}

void MainComponent::mouseDown(const MouseEvent &event) {
    if (event.mods.isRightButtonDown()){

        // Right-click menu
        PopupMenu m;
        menuPos = getMouseXYRelative();

        PopupMenu test = getEffectSelectMenu();
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
            /*auto testEffect = static_cast<EffectVT*>(effectsTree.getChild(-1).getProperty("Effect"));

            // Create Effect
            std::cout << "Test result: " << (testEffect != nullptr) << newLine;
            std::cout << testEffect->getName() << newLine;

            // Check and use child effect if selected
            if (auto g = dynamic_cast<GUIEffect*>(event.originalComponent)){
                auto parentTree = getTreeFromComponent(g, "GUI");
                if (parentTree.isValid())
                    parentTree.appendChild(testEffect->getTree(), &undoManager);
                else
                    std::cout << "Error: unable to retrieve tree from component.";
            } else {
                effectsTree.appendChild(testEffect->getTree(), &undoManager);
            }*/
        } else if (result == 2) {
            // Show settings
            deviceSelectorComponent.setVisible(!deviceSelectorComponent.isVisible());
        } else if (result == 3) {
            // Start audio
            initialiseGraph();
        }
    }
    Component::mouseDown(event);
}

//==============================================================================

void MainComponent::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    // Add ParameterGroup
    // Add to AudioProcessorGraph

    if (childWhichHasBeenAdded.getType() == ID_EFFECT_TREE){
        auto effectGui = static_cast<GUIEffect*>(childWhichHasBeenAdded.getProperty(ID_EFFECT_GUI).getObject());

        if (parentTree.getType() == ID_EFFECT_TREE){
            auto parentEffectGui = static_cast<GUIEffect*>(parentTree.getProperty(ID_EFFECT_GUI).getObject());
            parentEffectGui->addAndMakeVisible(effectGui);
        } else {
            addAndMakeVisible(effectGui);
        }

        effectGui->setCentrePosition(menuPos - effectGui->getParentComponent()->getPosition());
    }
}

/**
 * //TODO Create or find a "Property Component" type to replace GUIEffect* cast with for open-type usage
 * Function iterates upwards through component hierarchy, then down through matching tree hierarchy
 * @param g property of valuetree that is searched for
 * @param name name of property to search for (throughout the tree)
 * @return final valuetree match that contains property g
 */
ValueTree MainComponent::getTreeFromComponent(Component *g, String name) {
    // Iterate upwards through parents populating parentArray
    Array<Component*> parentArray;
    auto component = g;
    do {
        parentArray.add(component);
        component = component->getParentComponent();
    } while (component->getComponentID() != "MainWindow");

    // Iterate down from TreeTop checking at every step for a match
    ValueTree vt = effectsTree;
    for (int i = parentArray.size()-1; i >= 0 ; i--)
    {
        if (auto p = dynamic_cast<GUIEffect*>(parentArray[i]))
            vt = vt.getChildWithProperty(name, p);
        else return ValueTree(); // return invalid valuetree on failure
    }
    return vt;
}

//==============================================================================


void MainComponent::initialiseGraph()
{
    for (auto i = 0; i < effectsTree.getNumChildren(); i++){
        std::cout << effectsTree.getChild(i).getProperty("Name").toString() << newLine;
        auto node = static_cast<AudioProcessorGraph::Node*>(effectsTree.getChild(i).getProperty("Node").getObject());
        // add the mothafucka to da graph, dawg
        //TODO
        node->getProcessor()->setPlayConfigDetails(processorGraph->getMainBusNumInputChannels(),
                                                processorGraph->getMainBusNumOutputChannels(),
                                                processorGraph->getSampleRate(),
                                                processorGraph->getBlockSize());
    }

    connectAudioNodes();
}

void MainComponent::connectAudioNodes()
{
    // Start and End Nodes default
    if (effectsTree.getNumChildren() == 0){
        std::cout << "No children" << newLine;
        return;
    } else {
        // TEST: check state
        //TODO check effectsTree and AudioGraph state to see if everything is added correctly
        //TODO check in/out busses to fix audio.

        // Connect input and output nodes
        // Scan through top-level nodes for:
        // Input devices
        for (auto e : effectsTree.getChildWithProperty("Name", "Input Device")){
            auto node = static_cast<AudioProcessorGraph::Node*>(e.getProperty("Node").getObject());

        }
        // Output devices
        for (auto e : effectsTree.getChildWithProperty("Name", "Output Device")){
            auto node = static_cast<AudioProcessorGraph::Node*>(e.getProperty("Node").getObject());
        }

        auto source = effectsTree.getChild(0);
        for (int i = 1; i < effectsTree.getNumChildren(); i++) {
            auto dest = effectsTree.getChild(i);
            processorGraph->addConnection({
                      {static_cast<AudioProcessorGraph::Node*>(source.getProperty("Node").getObject())->nodeID},
                      {static_cast<AudioProcessorGraph::Node*>(dest.getProperty("Node").getObject())->nodeID}});

            source = effectsTree.getChild(i);
        }

        std::cout << "In/Out connections: " << newLine;
        for (auto i : processorGraph->getConnections()){
            std::cout << "Source: " << effectsTree.getChildWithProperty("Node",
                    processorGraph->getNodeForId(i.source.nodeID)).getProperty("Name").toString() << newLine;
            std::cout << "Dest: " << effectsTree.getChildWithProperty("Node",
                    processorGraph->getNodeForId(i.destination.nodeID)).getProperty("Name").toString() << newLine;
        }
    }
}

void MainComponent::updateGraph() {
    // Stop graph
    // Reconnect graph
    // Start graph
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