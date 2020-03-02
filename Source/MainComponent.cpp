/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() :
        effectsTree("TreeTop")
{
    setComponentID("MainWindow");
    setName("MainWindow");
    setSize (600, 400);

    // Manage EffectsTree
    effectsTree.addListener(this);



}

MainComponent::~MainComponent()
{
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
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::mouseDown(const MouseEvent &event) {
    if (event.mods.isRightButtonDown()){
        // Right-click menu
        PopupMenu m;
        m.addItem(1, "Create Effect");
        int result = m.show();

        if (result == 0) {
            // Menu ignored
        } else if (result == 1) {
            // Create Effect
            EffectVT::Ptr testEffect = new EffectVT();
            std::cout << "ref count: " << testEffect.get()->getReferenceCount() << newLine;
            effectsTree.appendChild(testEffect->getTree(), &undoManager);
        }

    }

    Component::mouseDown(event);
}

void MainComponent::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    if (childWhichHasBeenAdded.getType() == ID_EFFECT_TREE){
        auto effectGui = static_cast<GUIEffect*>(childWhichHasBeenAdded.getProperty(ID_EFFECT_GUI).getObject());
        addAndMakeVisible(effectGui);
    }
}
/*
void MainComponent::addEffect(GUIEffect::Ptr effectPtr)
{
    effectsArray.add(effectPtr.get());
}*/