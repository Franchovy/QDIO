/*
  ==============================================================================

    Effector.cpp
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/

#include "Effector.h"

LineComponent* LineComponent::dragLine = nullptr;

//==============================================================================
GUIEffect::GUIEffect () : Component()
{
    setSize (100, 100);
}

GUIEffect::~GUIEffect()
{

}

//==============================================================================
void GUIEffect::paint (Graphics& g)
{
    //g.fillAll(Colours::purple);
    //g.fillAll (Colour (200,200,200));
    //Component::paint(g);
    //g.drawRect(outline);
}

void GUIEffect::resized()
{
    inputPortPos = inputPortStartPos;
    outputPortPos = outputPortStartPos;
    for (auto p : inputPorts){
        p->setCentrePosition(portIncrement, inputPortPos);
        inputPortPos += portIncrement;
    }
    for (auto p : outputPorts){
        p->setCentrePosition(getWidth() - portIncrement, outputPortPos);
        outputPortPos += portIncrement;
    }
}

void GUIEffect::mouseDown(const MouseEvent &event) {
    getParentComponent()->mouseDown(event);
    //Component::mouseDown(event);
}

void GUIEffect::mouseDrag(const MouseEvent &event) {
    getParentComponent()->mouseDrag(event);
}

void ConnectionPort::connect(ConnectionPort &otherPort) {
    if (otherPort.isInput ^ isInput){
        // Ports isInput are of different values
        new ConnectionLine(*this, otherPort);
    } else {

    }
}

void ConnectionPort::mouseDown(const MouseEvent &event) {
    std::cout << event.position.toString() << newLine;
    LineComponent::getDragLine()->start(this, event.getScreenPosition());
}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    LineComponent::getDragLine()->drag(event.getScreenPosition());
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    Component::mouseUp(event);
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
