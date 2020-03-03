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
        processorGraph(new AudioProcessorGraph())
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize (600, 400);

    // Manage EffectsTree
    effectsTree.addListener(this);

    // Manage Audio
    //auto inputDevice  = MidiInput::getDevices()  [MidiInput::getDefaultDeviceIndex()];
    //auto outputDevice = MidiOutput::getDevices() [MidiOutput::getDefaultDeviceIndex()];

    processorGraph->enableAllBuses();

    deviceManager.initialiseWithDefaultDevices (processorGraph->getMainBusNumInputChannels(),
            processorGraph->getMainBusNumOutputChannels());         // [1]
    deviceManager.addAudioCallback (&player);                  // [2]

    //deviceManager.setMidiInputEnabled (inputDevice, true);
    //deviceManager.addMidiInputCallback (inputDevice, &player); // [3]
    //deviceManager.setDefaultMidiOutput (outputDevice);

    initialiseGraph();
    player.setProcessor (processorGraph.get());

    hideDeviceSelector.onClick = [=](){
        deviceSelector.setVisible(false);
    };
    deviceSelectorGroup.setName("Poop");
    deviceSelectorGroup.setText("Dumpy");
    addAndMakeVisible(deviceSelectorGroup);
    deviceSelectorGroup.addAndMakeVisible(hideDeviceSelector);
    deviceSelectorGroup.addAndMakeVisible(deviceSelector);
}

MainComponent::~MainComponent()
{
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
    deviceSelectorGroup.setBounds(10,10,getWidth()-10, getHeight()-10);
    deviceSelector.setBounds(10,10,getWidth()-10, getHeight()-10);
}

void MainComponent::mouseDown(const MouseEvent &event) {
    if (event.mods.isRightButtonDown()){

        // Right-click menu
        PopupMenu m;
        menuPos = getMouseXYRelative();

        m.addItem(1, "Create Effect");
        if (deviceSelectorGroup.isVisible())
            m.addItem(2, "Hide Settings");
        else
            m.addItem(2, "Show Settings");

        int result = m.show();

        if (result == 0) {
            // Menu ignored
        } else if (result == 1) {
            // Create Effect
            EffectVT::Ptr testEffect = new EffectVT();
            // Check and use child effect if selected
            if (auto g = dynamic_cast<GUIEffect*>(event.originalComponent)){
                auto parentTree = getTreeFromComponent(g, "GUI");
                if (parentTree.isValid())
                    parentTree.appendChild(testEffect->getTree(), &undoManager);
                else
                    std::cout << "Error: unable to retrieve tree from component.";
            } else {
                effectsTree.appendChild(testEffect->getTree(), &undoManager);
            }
        } else if (result == 2) {
            // Show settings
            deviceSelectorGroup.setVisible(true);
        }

    }

    Component::mouseDown(event);
}

void MainComponent::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
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
/*
void MainComponent::addEffect(GUIEffect::Ptr effectPtr)
{
    effectsArray.add(effectPtr.get());
}*/


void MainComponent::initialiseGraph()
{
    processorGraph->clear();

    audioInputNode  = processorGraph->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
    audioOutputNode = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));

    connectAudioNodes();
}

void MainComponent::connectAudioNodes()
{
    for (int channel = 0; channel < 2; ++channel)
        processorGraph->addConnection ({ { audioInputNode->nodeID,  channel },
                                        { audioOutputNode->nodeID, channel } });
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    emptyMidiMessageBuffer = MidiBuffer(MidiMessage());

    processorGraph->setPlayConfigDetails (numInputChannels,
                                         numOutputChannels,
                                         sampleRate, samplesPerBlockExpected);

    processorGraph->prepareToPlay (sampleRate, samplesPerBlockExpected);

    initialiseGraph();
}

void MainComponent::releaseResources() {
    processorGraph->releaseResources();
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    // don't bother with dat shit
    //for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    //    buffer.clear (i, 0, buffer.getNumSamples());

    updateGraph();

    processorGraph->processBlock (*bufferToFill.buffer, emptyMidiMessageBuffer);
}

void MainComponent::updateGraph() {
    /*slot->getProcessor()->setPlayConfigDetails (processorGraph->getMainBusNumInputChannels(),
                                                processorGraph->getMainBusNumOutputChannels(),
                                                processorGraph->getSampleRate(),
                                                processorGraph->getBlockSize());*/
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