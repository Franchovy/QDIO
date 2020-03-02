/*
  ==============================================================================

    Effect.cpp
    Created: 2 Mar 2020 3:28:24pm
    Author:  maxime

  ==============================================================================
*/

#include "Effect.h"


//==============================================================================
GUIEffect::GUIEffect ()
{
    setSize (100, 100);

    // outline Rectangle
    outline.setWidth(5);
    outline.setSize(getWidth(), getHeight());
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

