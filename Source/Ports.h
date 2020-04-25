/*
  ==============================================================================

    Ports.h
    Created: 25 Apr 2020 11:41:27am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EffectGui.h"


/**
 * Base class - port to connect to other ports
 */
class ConnectionPort : public SelectHoverObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ConnectionPort>;

    ~ConnectionPort() = default;

    Point<int> centrePoint;

    void paint(Graphics &g) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    virtual bool canConnect(ConnectionPort::Ptr& other) = 0;

    bool isInput;

    void setOtherPort(ConnectionPort::Ptr newPort) { otherPort = newPort; }
    ConnectionPort::Ptr getOtherPort() { return otherPort; }

    bool isConnected() { return otherPort != nullptr; }

    enum ColourIDs {
        portColour = 0
    };

protected:
    ConnectionPort() {
        setColour(portColour, Colours::black);
    };

    ConnectionPort::Ptr otherPort = nullptr;

    Rectangle<int> hoverBox;
    Rectangle<int> outline;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionPort)
};

class AudioPort;
class InternalConnectionPort : public ConnectionPort
{
public:
    using Ptr = ReferenceCountedObjectPtr<InternalConnectionPort>;

    InternalConnectionPort(AudioPort* parent, bool isInput) : ConnectionPort() {
        audioPort = parent;
        this->isInput = isInput;

        hoverBox = Rectangle<int>(0,0,30,30);
        outline = Rectangle<int>(10,10,10,10);
        centrePoint = Point<int>(15,15);
        //setBounds(parent->getX(), parent->getY(), 30, 30);
        setBounds(0, 0, 30, 30);
    }

    bool canConnect(ConnectionPort::Ptr& other) override;
    AudioPort* audioPort;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalConnectionPort)
};

class AudioPort : public ConnectionPort
{
public:
    using Ptr = ReferenceCountedObjectPtr<AudioPort>;

    explicit AudioPort(bool isInput);

    bool hitTest(int x, int y) override {
        return hoverBox.contains(x,y);
    }

    const InternalConnectionPort::Ptr getInternalConnectionPort() const { return internalPort; }

    AudioProcessor::Bus* bus;
    InternalConnectionPort::Ptr internalPort;

    bool canConnect(ConnectionPort::Ptr& other) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPort)
};
