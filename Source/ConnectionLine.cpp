/*
  ==============================================================================

    ConnectionLine.cpp
    Created: 24 Apr 2020 10:45:08am
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionLine.h"
#include "Effect.h"
#include "ConnectionGraph.h"


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

    addComponentListener(ConnectionGraph::getInstance());
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

    ConnectionPort *originPort;
    if (inPort == nullptr || outPort == nullptr) {
        // New Line
        jassert (inPort != nullptr || outPort != nullptr);
        originPort = (inPort != nullptr) ? inPort.get() : outPort.get();

        inPos = getParentComponent()->getLocalPoint(originPort, originPort->centrePoint);
        outPos = getParentComponent()->getLocalPoint(event.eventComponent, event.getPosition());

        setBounds(Rectangle<int>(inPos, outPos));

        SelectHoverObject::resetHoverObject();
    } else {
        // Reconnect line
        // Call undomanager new op
        getParentComponent()->postCommandMessage(9);
    }
}

void ConnectionLine::mouseDrag(const MouseEvent &event) {
    SelectHoverObject::mouseDrag(event.getEventRelativeTo(this));

    // If line is already connected
    if (inPort != nullptr && outPort != nullptr) {
        // Disconnect line
        if (event.getDistanceFromDragStart() > 15) {
            auto inPortPos = getLocalPoint(inPort.get(), inPort->centrePoint);
            auto outPortPos = getLocalPoint(outPort.get(), outPort->centrePoint);
            auto mouseClickPos = getLocalPoint(event.eventComponent, event.getPosition());
            if (inPortPos.getDistanceFrom(mouseClickPos) > outPortPos.getDistanceFrom(mouseClickPos)) {
                // outPort to change
                unsetPort(outPort.get());
            } else {
                // inPort to change
                unsetPort(inPort.get());
            }
        }
    }

    if (inPort == nullptr) {
        inPos = getParentComponent()->getLocalPoint(event.eventComponent, event.getPosition());
    } else if (outPort == nullptr) {
        outPos = getParentComponent()->getLocalPoint(event.eventComponent, event.getPosition());
    }
    setBounds(Rectangle<int>(inPos, outPos));

    auto oldPort = dynamic_cast<ConnectionPort*>(getHoverObject());

    if (auto obj = getDragIntoObject()) {
        if (auto port = dynamic_cast<ConnectionPort*>(obj)) {
            if (canConnect(port)) {
                setHoverObject(port);
                port->repaint();
            } else {
                resetHoverObject();
            }
        } else {
            resetHoverObject();
        }
    } else {
        if (oldPort != nullptr && ! oldPort->contains(oldPort->getMouseXYRelative())) {
            resetHoverObject();
        }
    }
    if (oldPort != nullptr) {
        oldPort->repaint();
    }
}

void ConnectionLine::mouseUp(const MouseEvent &event) {
    if (auto port = dynamic_cast<ConnectionPort*>(getHoverObject())) {
        setPort(port);
    }

    endDragHoverDetect();
    SelectHoverObject::mouseUp(event.getEventRelativeTo(this));

    getParentComponent()->removeMouseListener(this);

    if (event.getDistanceFromDragStart() > 15) {
        if (inPort != nullptr && outPort != nullptr) {
            //connect();
        } else {
            // Cancel drag
            getParentComponent()->removeChildComponent(this);
        }
    }
}

void ConnectionLine::resized() {
    if (getHeight() < 15) {
        setBounds(getBounds().withHeight(15));
    }
    if (getWidth() < 15) {
        setBounds(getBounds().withWidth(15));
    }

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
        inPort->setOtherPort(outPort.get());
        outPort->setOtherPort(inPort.get());

        setBounds(Rectangle<int>(inPos, outPos));

        line.setStart(getLocalPoint(getParentComponent(), inPos));
        line.setEnd(getLocalPoint(getParentComponent(), outPos));

        // Connect functionality

        auto inParamPort = dynamic_cast<ParameterPort*>(inPort.get());
        auto outParamPort = dynamic_cast<ParameterPort*>(outPort.get());

        // Parameter connection case
        if (inParamPort != nullptr && outParamPort != nullptr) {
            Effect::connectParameters(*this);
            type = parameter;
        }
        // Audio connection case
        else {
            jassert(dynamic_cast<AudioPort*>(inPort.get())
                 || dynamic_cast<InternalConnectionPort*>(inPort.get()));
            jassert(dynamic_cast<AudioPort*>(outPort.get())
                 || dynamic_cast<InternalConnectionPort*>(outPort.get()));

            Effect::connectAudio(*this);
            type = audio;
        }

        connected = true;
        setEnabled(true);
        return true;

    } else {
        return false;
    }

    return false;
}

void ConnectionLine::disconnect(ConnectionPort* port) {
    connected = false;

    // Disconnect functionality

    auto inParamPort = dynamic_cast<ParameterPort*>(inPort.get());
    auto outParamPort = dynamic_cast<ParameterPort*>(outPort.get());

    // Parameter connection case
    if (inParamPort != nullptr && outParamPort != nullptr) {
        Effect::disconnectParameters(*this);
    }
        // Audio connection case
    else if ((dynamic_cast<AudioPort*>(inPort.get())
                || dynamic_cast<InternalConnectionPort*>(inPort.get()))
             && (dynamic_cast<AudioPort*>(outPort.get())
                || dynamic_cast<InternalConnectionPort*>(outPort.get())))
    {
        Effect::disconnectAudio(*this);
    }


    if (port != nullptr) {
        unsetPort(port);
    } else {
        if (inPort != nullptr) {
            inPort->setOtherPort(nullptr);
        }
        if (outPort != nullptr) {
            outPort->setOtherPort(nullptr);
        }
    }


    setEnabled(false);
}

bool ConnectionLine::setPort(ConnectionPort *port) {
    if (port->isInput) {
        inPort = port;
        inPort->getParentComponent()->addComponentListener(this);
    } else {
        outPort = port;
        outPort->getParentComponent()->addComponentListener(this);
    }

    if (!connected && (inPort != nullptr && outPort != nullptr)) {
        return connect();
    } else {
        return true;
    }
}

void ConnectionLine::unsetPort(ConnectionPort *port) {
    jassert(port == inPort || port == outPort);

    if (connected) {
        disconnect(port);
    } else if (port == inPort) {
        inPort->getParentComponent()->removeComponentListener(this);
        inPort->setOtherPort(nullptr);
        if (outPort != nullptr) {
            outPort->setOtherPort(nullptr);
        }

        inPort = nullptr;
    } else if (port == outPort) {
        outPort->getParentComponent()->removeComponentListener(this);
        outPort->setOtherPort(nullptr);
        if (inPort != nullptr) {
            inPort->setOtherPort(nullptr);
        }

        outPort = nullptr;
    } else {
        jassertfalse;
    }
}

bool ConnectionLine::isConnected() const {
    return connected;
}

void ConnectionLine::reconnect(ConnectionPort *newInPort, ConnectionPort *newOutPort) {
    jassert(newInPort->isInput && ! newOutPort->isInput);

    auto oldOutPort = outPort.get();
    unsetPort(oldOutPort);
    setPort(newOutPort);

    auto newLine = new ConnectionLine();
    getParentComponent()->addAndMakeVisible(newLine);
    newLine->setPort(oldOutPort);
    newLine->setPort(newInPort);
}

/**
 * Get Line relative to parent
 * @return Juce line object with points relative to parent
 */
Line<int> ConnectionLine::getLine() {
    return Line<int>(line.getStart() + getPosition(), line.getEnd() + getPosition());
}