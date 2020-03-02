/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "GUIEffect.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public Component, public ValueTree::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void mouseDown(const MouseEvent &event) override;

    //void addEffect(GUIEffect::Ptr effectPtr);

private:
    ValueTree effectsTree;
    void valueTreeChildAdded (ValueTree &parentTree, ValueTree &childWhichHasBeenAdded);

    UndoManager undoManager;

    // GUI Test
    GUIEffect test;


    // StringRefs - move these to Includer file
    Identifier ID_TREE_TOP = "Treetop";
    Identifier ID_EFFECT_TREE = "effectTree";
    Identifier ID_EFFECT_GUI = "GUI";

    //void createEffect(GUIEffect::Ptr parent = nullptr);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
