/*
  ==============================================================================

    Effector.cpp
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/

#include "Effector.h"


//==============================================================================
GUIEffect::GUIEffect ()
{
    setSize (100, 100);

    // outline Rectangle
    outline.setWidth(5);
    outline.setSize(getWidth(), getHeight());

    // Add resizer
    addAndMakeVisible(resizer);

}

GUIEffect::~GUIEffect()
{

}

//==============================================================================
void GUIEffect::paint (Graphics& g)
{
    g.fillAll (Colour (255,255,255));

    g.setColour(Colour(0,0,0));
    g.drawRect(outline);
}

void GUIEffect::resized()
{
    outline.setSize(getWidth(), getHeight());
}

void GUIEffect::mouseDown(const MouseEvent &event) {
    dragger.startDraggingComponent(this, event);

    if (event.mods.isRightButtonDown())
        getParentComponent()->mouseDown(event);
    Component::mouseDown(event);
}

void GUIEffect::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);
    Component::mouseDrag(event);
}




//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GUIEffect" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

