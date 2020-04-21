#include "EffectScene.h"

const Identifier EffectScene::IDs::DeviceManager = "deviceManager";

//==============================================================================
EffectScene::EffectScene() :
        EffectTreeBase(EFFECTSCENE_ID)
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize(4000, 4000);
/*
    // RANDOM LIST TEST
    int list1[] = {1,2,3};
    int list2[] = {4,5,6};
    for (auto e : (list1; list2)) {
        std::cout << e << newLine;
    }*/

    // Set up static members

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
    tree.addListener(this);

#define BACKGROUND_IMAGE
    bg = ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);

    //========================================================================================
    // MIDI example code

    //auto inputDevice  = MidiInput::getDevices()  [MidiInput::getDefaultDeviceIndex()];
    //auto outputDevice = MidiOutput::getDevices() [MidiOutput::getDefaultDeviceIndex()];

    //deviceManager.setMidiInputEnabled (inputDevice, true);
    //deviceManager.addMidiInputCallback (inputDevice, &processorPlayer); // [3]
    //deviceManager.setDefaultMidiOutput (outputDevice);


    //==============================================================================
    // Main component popup menu
    PopupMenu createEffectSubmenu = getEffectSelectMenu();
    mainMenu.addSubMenu("Create Effect", createEffectSubmenu);

    //==============================================================================
    // Load Effects if there are any saved
    tree.setProperty(EffectTreeBase::IDs::effectTreeBase, this, nullptr);
    if (getAppProperties().getUserSettings()->getValue(KEYNAME_LOADED_EFFECTS).isNotEmpty()) {
        auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);

        std::cout << loadedEffectsData->toString() << newLine;
        std::cout << loadedEffectsData->getTagName() << newLine;

        ValueTree effectLoadDataTree = ValueTree::fromXml(*loadedEffectsData);

        loadEffect(tree, effectLoadDataTree);

        //tree.appendChild(loadedEffects, nullptr);
    }
}

EffectScene::~EffectScene()
{
    audioGraph.clear();
    processorPlayer.setProcessor(nullptr);
    deviceManager.closeAudioDevice();

    undoManager.clearUndoHistory();
}

//==============================================================================
void EffectScene::paint (Graphics& g)
{
#ifdef BACKGROUND_IMAGE
    g.drawImage(bg, getBounds().toFloat());
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
    EffectTreeBase::mouseDown(event);
}


void EffectScene::mouseUp(const MouseEvent &event) {
    // TODO implement all of this locally
    if (lasso.isVisible())
        lasso.endLasso();

    // Open menu - either right click or left click (for mac)
    if (event.getDistanceFromDragStart() < 10
        && (event.mods.isRightButtonDown() ||
            event.mods.isCtrlDown())) {
            callMenu(mainMenu);
    }

    EffectTreeBase::mouseUp(event);
}

bool EffectScene::keyPressed(const KeyPress &key)
{
    std::cout << "Key press: " << key.getKeyCode() << newLine;
    EffectTreeBase::keyPressed(key);
}

void EffectScene::deleteEffect(Effect* e) {
    delete e;
}

void EffectScene::storeState() {
    // Save screen state
    //auto savedState = toStorage(tree);

    auto savedState = storeEffect(tree).createXml();

    std::cout << "Save state: " << savedState->toString() << newLine;
    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, savedState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();
}

void EffectScene::updateChannels() {
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
}
/*
void EffectScene::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) { 
    getParentComponent()->mouseWheelMove(event, wheel);
}*/


void ComponentSelection::itemSelected(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<Effect*>(c.get())) {
        SelectHoverObject::addSelectObject(e);
    }
    c->repaint();
}

void ComponentSelection::itemDeselected(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<Effect*>(c.get())) {
        SelectHoverObject::removeSelectObject(e);
    }
    c->repaint();
}
