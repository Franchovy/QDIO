/*
  ==============================================================================

    Effect.h
    Created: 2 Mar 2020 3:28:24pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"



/**

    Describe your class and how it works here!
*/


class GUIEffect  : public Component, public ReferenceCountedObject
{
public:
    GUIEffect ();
    ~GUIEffect() override;

    using Ptr = ReferenceCountedObjectPtr<GUIEffect>;

    void paint (Graphics& g) override;
    void resized() override;



private:
    Rectangle<float> outline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIEffect)
};


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
