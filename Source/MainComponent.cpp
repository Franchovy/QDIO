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
    for (int i = 0; i < effectsTree.getNumChildren(); i++)
        effectsTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject()->decReferenceCount();

    //std::cout << effectsTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject()->getReferenceCount();
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
        menuPos = getMouseXYRelative();
        m.addItem(1, "Create Effect");
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