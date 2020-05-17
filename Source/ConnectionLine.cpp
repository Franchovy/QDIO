/*
  ==============================================================================

    ConnectionLine.cpp
    Created: 24 Apr 2020 10:45:08am
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionLine.h"
#include "Effect.h"


const Identifier ConnectionLine::IDs::CONNECTION_ID = "connection";
const Identifier ConnectionLine::IDs::ConnectionLineObject = "connectionLineObject";
const Identifier ConnectionLine::IDs::InPort = "inport";
const Identifier ConnectionLine::IDs::OutPort = "outport";
const Identifier ConnectionLine::IDs::AudioConnection = "audioConnection";



ConnectionLine::ConnectionLine() {
    setRepaintsOnMouseActivity(true);
    setEnabled(false);
    inPort = nullptr;
    outPort = nullptr;
}

ConnectionLine::~ConnectionLine() {
    if (inPort != nullptr) {
        inPort->setOtherPort(nullptr);
        inPort->removeComponentListener(this);
    }
    if (outPort != nullptr) {
        outPort->setOtherPort(nullptr);
        outPort->removeComponentListener(this);
    }
}

void ConnectionLine::paint(Graphics &g) {
    Path p;
    int thiccness;

    if (hoverMode || selectMode) {
        g.setColour(Colours::blue);
        thiccness = 3;
    } else {
        g.setColour(Colours::whitesmoke);
        thiccness = 2;
    }

    p.addLineSegment(line.toFloat(),thiccness);

    g.fillPath(p);
}


void ConnectionLine::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    Point<int> inPos, outPos;
    if (inPort != nullptr) {
        inPos = getParentComponent()->getLocalPoint(inPort.get(), inPort->centrePoint);
    }
    if (outPort != nullptr) {
        outPos = getParentComponent()->getLocalPoint(outPort.get(), outPort->centrePoint);
    }
    setBounds(Rectangle<int>(inPos, outPos));

    line.setStart(getLocalPoint(getParentComponent(), inPos));
    line.setEnd(getLocalPoint(getParentComponent(), outPos));
}

bool ConnectionLine::hitTest(int x, int y) {
    bool i,j;
    getInterceptsMouseClicks(i, j);
    if (! i) {
        return false;
    }

    auto d1 = line.getStart().getDistanceFrom(Point<int>(x,y));
    auto d2 = line.getEnd().getDistanceFrom(Point<int>(x,y));
    auto d = d1 + d2 - line.getLength();

    if (d < 3) {
        return true;
    } else {
        return false;
    }
}


void ConnectionLine::mouseDown(const MouseEvent &event) {
    startDragHoverDetect();
    SelectHoverObject::mouseDown(event.getEventRelativeTo(this));

    ConnectionPort *port;
    if (inPort != nullptr && outPort != nullptr) {
        // Line already connected

        auto inPortPos = getLocalPoint(inPort.get(), inPort->centrePoint);
        auto outPortPos = getLocalPoint(outPort.get(), outPort->centrePoint);
        auto mouseClickPos = getLocalPoint(event.eventComponent, event.getPosition());
        if (inPortPos.getDistanceFrom(mouseClickPos) > outPortPos.getDistanceFrom(mouseClickPos)) {
            // inPort to change
            port = inPort.get();
        } else {
            // outPort to change
            port = outPort.get();
        }
    } else {
        // New Line
        jassert (inPort != nullptr || outPort != nullptr);
        port = (inPort != nullptr) ? inPort.get() : outPort.get();
    }

    inPos = getParentComponent()->getLocalPoint(port, port->centrePoint);
    outPos = getParentComponent()->getLocalPoint(event.eventComponent, event.getPosition());

    setBounds(Rectangle<int>(inPos, outPos));

    SelectHoverObject::resetHoverObject();
}

void ConnectionLine::mouseDrag(const MouseEvent &event) {
    SelectHoverObject::mouseDrag(event.getEventRelativeTo(this));

    outPos = getParentComponent()->getLocalPoint(event.eventComponent, event.getPosition());
    setBounds(Rectangle<int>(inPos, outPos));

    if (auto obj = getDragIntoObject()) {
        if (auto port = dynamic_cast<ConnectionPort*>(obj)) {
            if (canConnect(port)) {
                setHoverObject(port);
            }
        }
    }
}

void ConnectionLine::mouseUp(const MouseEvent &event) {
    if (auto port = dynamic_cast<ConnectionPort*>(getHoverObject())) {
        setPort(port);
    }

    endDragHoverDetect();
    SelectHoverObject::mouseUp(event.getEventRelativeTo(this));

    getParentComponent()->removeMouseListener(this);

    if (inPort != nullptr && outPort != nullptr) {
        connect();
    } else {
        // Cancel drag
        setVisible(false);
    }
}

void ConnectionLine::parentHierarchyChanged() {
    auto parent = getParentComponent();
    if (parent != nullptr) {
        setBounds(parent->getBounds());
    }
    Component::parentHierarchyChanged();
}

void ConnectionLine::resized() {
    line.setStart(getLocalPoint(getParentComponent(), inPos));
    line.setEnd(getLocalPoint(getParentComponent(), outPos));

    Component::resized();
}

bool ConnectionLine::canConnect(const ConnectionPort *port) const {
    if (inPort != nullptr) {
        return inPort->canConnect(port);
    } else if (outPort != nullptr) {
        return outPort->canConnect(port);
    }
    jassertfalse;
    return false;
}

bool ConnectionLine::connect() {
    if (inPort != nullptr && outPort != nullptr) {
        // Connect listeners and port components
        inPos = getParentComponent()->getLocalPoint(inPort.get(), inPort->centrePoint);
        outPos = getParentComponent()->getLocalPoint(outPort.get(), outPort->centrePoint);

        inPort->getParentComponent()->addComponentListener(this);
        outPort->getParentComponent()->addComponentListener(this);
        inPort->setOtherPort(outPort);
        outPort->setOtherPort(inPort);

        setBounds(Rectangle<int>(inPos, outPos));

        // Connect functionality

        auto inParamPort = dynamic_cast<ParameterPort*>(inPort.get());
        auto outParamPort = dynamic_cast<ParameterPort*>(outPort.get());

        // Parameter connection case
        if (inParamPort != nullptr && outParamPort != nullptr) {
            //Effect::connectParameters
        }
        // Audio connection case
        else {
            jassert(dynamic_cast<AudioPort*>(inPort.get())
                 || dynamic_cast<InternalConnectionPort*>(inPort.get()));
            jassert(dynamic_cast<AudioPort*>(outPort.get())
                 || dynamic_cast<InternalConnectionPort*>(outPort.get()));

            Effect::connectAudio(*this);
        }

        isConnected = true;
        setEnabled(true);
        return true;

    } else {
        return false;
    }

    return false;
}

void ConnectionLine::disconnect() {
    isConnected = false;

    // Disconnect functionality

    auto inParamPort = dynamic_cast<ParameterPort*>(inPort.get());
    auto outParamPort = dynamic_cast<ParameterPort*>(outPort.get());

    // Parameter connection case
    if (inParamPort != nullptr && outParamPort != nullptr) {
        //Effect::disconnectParameters
    }
        // Audio connection case
    else {
        jassert(dynamic_cast<AudioPort*>(inPort.get())
                || dynamic_cast<InternalConnectionPort*>(inPort.get()));
        jassert(dynamic_cast<AudioPort*>(outPort.get())
                || dynamic_cast<InternalConnectionPort*>(outPort.get()));

        Effect::disconnectAudio(*this);
    }

    setEnabled(false);
}

void ConnectionLine::setPort(ConnectionPort *port) {
    if (port->isInput) {
        inPort = port;
    } else {
        outPort = port;
    }
}

void ConnectionLine::unsetPort(ConnectionPort *port) {
    if (port == inPort) {
        inPort->getParentComponent()->addComponentListener(this);
        inPort->setOtherPort(nullptr);
        outPort->setOtherPort(nullptr);

        inPort = nullptr;
    } else if (port == outPort) {
        outPort->getParentComponent()->addComponentListener(this);
        outPort->setOtherPort(nullptr);
        inPort->setOtherPort(nullptr);

        outPort = nullptr;
    } else {
        jassertfalse;
    }

    if (isConnected && (inPort == nullptr || outPort == nullptr)) {
        std::cout << "disconnect!" << newLine;
        disconnect();
    }
}
