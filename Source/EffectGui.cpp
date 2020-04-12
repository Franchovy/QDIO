/*
  ==============================================================================

    EffectGui.cpp
    Created: 5 Apr 2020 11:20:04am
    Author:  maxime

  ==============================================================================
*/

#include "EffectGui.h"

SelectHoverObject* SelectHoverObject::hoverComponent = nullptr;
ComponentSelection SelectHoverObject::selected;
ReferenceCountedArray<SelectHoverObject> SelectHoverObject::componentsToSelect;
LineComponent* LineComponent::dragLine = nullptr;

const Identifier ConnectionLine::IDs::CONNECTION_ID = "connection";
const Identifier ConnectionLine::IDs::ConnectionLineObject = "connectionLineObject";
const Identifier ConnectionLine::IDs::InPort = "inport";
const Identifier ConnectionLine::IDs::OutPort = "outport";
const Identifier ConnectionLine::IDs::AudioConnection = "audioConnection";


//==============================================================================
// Line Component methods

/**
 * @param event modified mouseEvent to use mainComponent coords.
 */
void LineComponent::mouseDown(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);

    if (port1 = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        p1 = getLocalPoint(port1, port1->centrePoint);
    }

    p2 = thisEvent.getPosition();

    line.setStart(p1);
    line.setEnd(p2);

    setVisible(true);
    repaint();

    getParentComponent()->mouseDown(event);
}

void LineComponent::mouseDrag(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);
    p2 = thisEvent.getPosition();

    line.setEnd(p2);
    repaint();

    // Pass hover detection to EffectScene
    getParentComponent()->mouseDrag(thisEvent);
}

void LineComponent::mouseUp(const MouseEvent &event) {
    setVisible(false);

    // Pass this event to EffectScene
    auto eventMain = event.getEventRelativeTo(this).withNewPosition(
            event.getPosition() - getPosition()
    );
    getParentComponent()->mouseUp(eventMain);
}

void LineComponent::convert(ConnectionPort *port2) {
    if (port1 != nullptr) {
        // Connect port1 to port2

    }
}

//==============================================================================
// GuiEffect methods


//=================================================================================
// AudioPort methods

AudioPort::AudioPort(bool isInput) : ConnectionPort()
        , internalPort(new InternalConnectionPort(this, !isInput))
{

    hoverBox = Rectangle<int>(0,0,60,60);
    outline = Rectangle<int>(20, 20, 20, 20);
    centrePoint = Point<int>(30,30);
    setBounds(0,0,60, 60);

    this->isInput = isInput;
}

bool AudioPort::canConnect(ConnectionPort::Ptr& other) {
    if (this->isInput == other->isInput)
        return false;

    // Connect to AudioPort of mutual parent
    return (dynamic_cast<AudioPort *>(other.get())
            && other->getParentComponent()->getParentComponent() == this->getParentComponent()->getParentComponent())
           // Connect to ICP of containing parent effect
           || (dynamic_cast<InternalConnectionPort *>(other.get())
               && other->getParentComponent() == this->getParentComponent()->getParentComponent());
}

void AudioPort::mouseDown(const MouseEvent &event) {
    getParentComponent()->getParentComponent()->addAndMakeVisible(LineComponent::getDragLine());
    ConnectionPort::mouseDown(event);
}

// ==============================================================================
// Resizer methods

void Resizer::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);

    getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                  startPos.y + event.getDistanceFromDragStartY());
    Component::mouseDrag(event);
}

void ConnectionLine::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    line.setStart(getLocalPoint(inPort.get(), inPort->centrePoint));
    line.setEnd(getLocalPoint(outPort.get(), outPort->centrePoint));

    repaint();
}

bool ConnectionLine::hitTest(int x, int y) {
    auto d1 = line.getStart().getDistanceFrom(Point<int>(x,y));
    auto d2 = line.getEnd().getDistanceFrom(Point<int>(x,y));
    auto d = d1 + d2 - line.getLength();

    if (d < 7) {
        return true;
    } else {
        return false;
    }
}

ConnectionLine::ConnectionLine(ConnectionPort &p1, ConnectionPort &p2) {
    // Remember that input ports are the line output and vice versa
    if (!p1.isInput) {
        inPort = &p1;
        outPort = &p2;
    } else {
        inPort = &p2;
        outPort = &p1;
    }

    line = Line<int>(inPort->getParentComponent()->getPosition() + inPort->getPosition() + inPort->centrePoint,
                     outPort->getParentComponent()->getPosition() + outPort->getPosition() + outPort->centrePoint);

    inPort->setOtherPort(outPort);
    outPort->setOtherPort(inPort);
    inPort->getParentComponent()->addComponentListener(this);
    outPort->getParentComponent()->addComponentListener(this);

    setBounds(0,0,getParentWidth(),getParentHeight());
}

void ConnectionPort::mouseDown(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDown(event);
}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDrag(event);
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseUp(event);
}

void ConnectionPort::paint(Graphics &g) {
    g.setColour(Colours::black);
    //rectangle.setPosition(10,10);
    g.drawRect(outline,2);

    // Hover rectangle
    g.setColour(Colours::blue);
    Path drawPath;
    drawPath.addRoundedRectangle(hoverBox, 10, 10);
    PathStrokeType strokeType(3);

    if (hoverMode) {
        float thiccness[] = {2, 3};
        strokeType.createDashedStroke(drawPath, drawPath, thiccness, 2);
        g.strokePath(drawPath, strokeType);
    }
}


bool InternalConnectionPort::canConnect(ConnectionPort::Ptr& other) {
    // Return false if the port is AP and belongs to the same parent
    return !(dynamic_cast<AudioPort *>(other.get())
             && this->getParentComponent() == other->getParentComponent());
}

void InternalConnectionPort::mouseDown(const MouseEvent &event) {
    getParentComponent()->addAndMakeVisible(LineComponent::getDragLine());
    ConnectionPort::mouseDown(event);
}


void SelectHoverObject::setHoverComponent(SelectHoverObject::Ptr item) {
    setHoverComponent(item.get());
}

void SelectHoverObject::setHoverComponent(SelectHoverObject* item) {
    resetHoverObject();

    if (item != nullptr) {

        item->hoverMode = true;
        hoverComponent = item;
        item->repaint();
    }
}

SelectHoverObject::SelectHoverObject() {
    componentsToSelect.add(this);
}

SelectHoverObject::~SelectHoverObject() {
    componentsToSelect.removeObject(this);

    if (hoverComponent == this){
        hoverComponent = nullptr;
    }
}

void SelectHoverObject::resetHoverObject() {
    if (hoverComponent != nullptr) {
        hoverComponent->hoverMode = false;
        hoverComponent->repaint();
        hoverComponent = nullptr;
    }
}

void SelectHoverObject::mouseEnter(const MouseEvent &event) {
    setHoverComponent(this);
}

void SelectHoverObject::mouseExit(const MouseEvent &event) {
    resetHoverObject();
}

void SelectHoverObject::setSelectMode(bool newSelectMode) {
    if (newSelectMode) {
        addSelectObject(this);
    } else {
        removeSelectObject(this);
    }
}

void SelectHoverObject::addSelectObject(SelectHoverObject *item) {
    selected.addToSelection(item);
}

void SelectHoverObject::removeSelectObject(SelectHoverObject *item) {
    selected.deselect(item);
}
