/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.4.7

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/


class GUIEffect  : public Component, public ReferenceCountedObject
{
public:
    //==============================================================================
    GUIEffect ();
    ~GUIEffect() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    using Ptr = ReferenceCountedObjectPtr<GUIEffect>;
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    Rectangle<float> outline;
    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIEffect)
};


//[EndFile]
class EffectVT : public ReferenceCountedObject
{
public:
    EffectVT() : effectTree("effectTree")
    {
        // Set this object as a property of the owned valuetree....
        effectTree.setProperty("effectVT", this, nullptr);
        // Set GUI object as property
        effectTree.setProperty("GUI", &gui, nullptr);
    }
    ~EffectVT()
    {
        effectTree.removeProperty("effectVT", nullptr);
    }

    using Ptr = ReferenceCountedObjectPtr<EffectVT>;

    ValueTree& getTree() {return effectTree;}

private:
    ValueTree effectTree;
    GUIEffect gui;

};
//[/EndFile]