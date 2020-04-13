#include "EffectScene.h"


const Identifier EffectScene::IDs::EFFECTSCENE_ID = "effectscene";

//==============================================================================
EffectScene::EffectScene() :
        EffectTreeBase(ID_TREE_TOP)
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize (4000, 4000);


    // Set up static members

    audioGraph.enableAllBuses();

    deviceManager.initialise(256, 256, getAppProperties().getUserSettings()->getXmlValue (KEYNAME_DEVICE_SETTINGS).get(), true);

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

}

EffectScene::~EffectScene()
{
    // Save screen state
    //auto savedState = getAppProperties().getUserSettings()->setXmlValue (KEYNAME_LOADED_EFFECTS);

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

    /*if (event.mods.isLeftButtonDown()) {
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
                newParent->getTree().appendChild(effect->getTree(), &undoManager);
            }
        }
    }*/
        
    // Open menu - either right click or left click (for mac)
    if (event.mods.isRightButtonDown() && event.getDistanceFromDragStart() < 10) {
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
