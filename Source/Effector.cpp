/*
  ==============================================================================

    Effector.cpp
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/

#include "Effector.h"

LineComponent* LineComponent::dragLine = nullptr;

/**
 * @param event modified mouseEvent to use mainComponent coords.
 */
void LineComponent::mouseDown(const MouseEvent &event) {
    std::cout << "Down" << newLine;
    port1 = dynamic_cast<ConnectionPort*>(event.originalComponent);
    p1 = port1->getMainCentrePos();
    p2 = event.getPosition();

    line.setStart(p1);
    line.setEnd(p2);
    //setBounds(Rectangle<int>(p1, p2));
    setVisible(true);
    repaint();
    //p2 = event.getPosition().toFloat();
    //Component::mouseDown(event);
}

void LineComponent::mouseDrag(const MouseEvent &event) {
    p2 = event.getPosition();

    line.setEnd(p2);
    repaint();

    if (getComponentAt(event.getPosition()) != 0)
        std::cout << "Component at: " << getParentComponent()->getComponentAt(event.getPosition())->getName() << newLine;


}

void LineComponent::mouseUp(const MouseEvent &event) {
    setVisible(false);
    auto c = getParentComponent()->getComponentAt(event.getPosition());
    auto port = dynamic_cast<ConnectionPort*>(c);

    if (port){
        // If mouseup is over port
        if (port->isInput ^ port1->isInput){
            lastConnectionLine = convert(port);
            // This calls the propertyChange update in MainComponent
            dragLineTree.setProperty("Connection", lastConnectionLine.get(), nullptr);
        } else {
            std::cout << "Connected wrong port types" << newLine;
        }
    } else {
        std::cout << "No connection made" << newLine;
    }
    //setVisible(false);
}

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
    auto newEvent = event.withNewPosition(event.getPosition() + getMainParentPos() + getPosition());
    LineComponent::getDragLine()->mouseDown(newEvent);//->start(this, getMainCentrePos(), event.getPosition() - getPosition());
}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    auto newEvent = event.withNewPosition(event.getPosition() + getMainParentPos() + getPosition());
    LineComponent::getDragLine()->mouseDrag(newEvent);//->drag(event.getPosition());
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    auto newEvent = event.withNewPosition(event.getPosition() + getMainParentPos() + getPosition());
    LineComponent::getDragLine()->mouseUp(newEvent);
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
