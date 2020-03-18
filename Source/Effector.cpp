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
}

void LineComponent::mouseUp(const MouseEvent &event) {
    setVisible(false);
    auto c = getParentComponent()->getComponentAt(event.getPosition());
    auto port = dynamic_cast<ConnectionPort*>(c);

    if (port){
        // If mouseup is over port
        if (port->isInput ^ port1->isInput){
            std::cout << "Port1: " << port1->isInput << newLine;
            std::cout << "Port2: " << port->isInput << newLine;

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
GUIEffect::GUIEffect (const MouseEvent &event, EffectVT* parentEVT) :
    EVT(parentEVT)
{
    addAndMakeVisible(resizer);

    //TODO assign the tree before creation of GUIEffect for this to work
    if (parentEVT->getTree().getParent().hasType(ID_EFFECT_VT)){
        auto sizeDef = dynamic_cast<GUIEffect*>(parentEVT->getTree().getParent()
                .getProperty(ID_EFFECT_GUI).getObject())->getWidth() / 3;
        setBounds(event.getPosition().x, event.getPosition().y, sizeDef, sizeDef);
    } else {
        setBounds(event.getPosition().x, event.getPosition().y, 200,200);
    }
}

GUIEffect::~GUIEffect()
{

}

void GUIEffect::insertEffectGroup() {

}

void GUIEffect::insertEffect() {

}

// Processor hasEditor? What to do if processor is a predefined plugin
void GUIEffect::setProcessor(AudioProcessor *processor) {
    // Set up ports based on processor buses
    int numInputBuses = processor->getBusCount(true );
    int numBuses = numInputBuses + processor->getBusCount(false);
    for (int i = 0; i < numBuses; i++){
        bool isInput = (i < numInputBuses) ? true : false;
        // Get bus from processor
        auto bus = isInput ? processor->getBus(true, i) :
                processor->getBus(false, i - numInputBuses);
        // Check channel number - if 0 ignore
        if (bus->getNumberOfChannels() == 0)
            continue;
        // Create port - giving audiochannelset info and isInput bool
        addPort(bus, isInput);
    }

    // Setup parameters

    // Update
    resized();
    repaint();
}



//==============================================================================
void GUIEffect::paint (Graphics& g)
{
    g.fillAll(Colours::whitesmoke);

    g.setColour(Colours::black);
    g.drawRect(0,0,getWidth(),getHeight(), 2);
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
    dragger.startDraggingComponent(this, event);

    if (event.mods.isRightButtonDown())
        getParentComponent()->mouseDown(event);
}

void GUIEffect::mouseDrag(const MouseEvent &event) {

/*    constrainer.setBoundsForComponent(this,
            Rectangle<int>(newX, newY, getWidth(), getHeight()),
                    false, false, false ,false);*/
    dragger.dragComponent(this, event, &constrainer);

    // Manual constraint
    auto newX = jlimit<int>(0, getParentWidth() - getWidth(), getX());
    auto newY = jlimit<int>(0, getParentHeight() - getHeight(), getY());

    if (newX != getX() || newY != getY())
        if (event.x < -(getWidth()/2) || event.y < -(getHeight()/2) ||
                event.x > (getWidth()*3/2) || event.y > (getHeight()*3/2) ){
            auto newPos = dragDetachFromParentComponent();
            newX = newPos.x;
            newY = newPos.y;
        }

    setTopLeftPosition(newX, newY);

}

void GUIEffect::mouseUp(const MouseEvent &event) {
    event.withNewPosition(event.getPosition());
    getParentComponent()->mouseUp(event);
}

void GUIEffect::moved() {
    for (auto i : inputPorts){
        i->moved();
    }
    for (auto i : outputPorts){
        i->moved();
    }
    Component::moved();
}

void GUIEffect::visibilityChanged() {
    // Set parent (Wrapper) visibility
    if (getParentComponent())
        getParentComponent()->setVisible(this->isVisible());
}

Point<int> GUIEffect::dragDetachFromParentComponent() {
    auto newPos = getPosition() + getParentComponent()->getPosition();
    auto parentParent = getParentComponent()->getParentComponent();
    getParentComponent()->removeChildComponent(this);
    parentParent->addAndMakeVisible(this);
    return newPos;
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

void ConnectionPort::moved() {
    if (line != nullptr){
        line->move(isInput, getPosition().toFloat());
    }
    Component::moved();
}


//==============================================================================
// EffectVT static member variable

AudioProcessorGraph* EffectVT::graph = nullptr;

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

void Resizer::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);

    getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                      startPos.y + event.getDistanceFromDragStartY());
    Component::mouseDrag(event);
}
