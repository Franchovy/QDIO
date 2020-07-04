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

class Effect;

/**
 * Base class - port to connect to other ports
 */
class ConnectionPort : public SelectHoverObject, public SettableTooltipClient
{
public:
    using Ptr = ReferenceCountedObjectPtr<ConnectionPort>;

    ~ConnectionPort() = default;

    Point<int> centrePoint;

    void paint(Graphics &g) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    bool canDragInto(const SelectHoverObject *other, bool isRightClickDrag = false) const override;
    bool canDragHover(const SelectHoverObject *other, bool isRightClickDrag = false) const override;

    virtual bool canConnect(const ConnectionPort* other) const = 0;
    virtual Component* getDragLineParent() = 0;
    virtual Effect* getParentEffect();

    bool isInput;
    bool isInternal = false;

    void setOtherPort(ConnectionPort* newPort);
    ConnectionPort::Ptr getOtherPort();

    bool isConnected() { return otherPort != nullptr; }

    enum ColourIDs {
        portColour = 0
    };

    ConnectionPort* getLinkedPort() const;
    void setLinkedPort(ConnectionPort* port);

protected:
    ConnectionPort();

    ConnectionPort::Ptr otherPort = nullptr;

    Rectangle<int> hoverBox;
    Rectangle<int> outline;

    ConnectionPort* linkedPort = nullptr;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionPort)
};

class ParameterPort : public ConnectionPort
{
public:
    using Ptr = ReferenceCountedObjectPtr<ParameterPort>;
    ParameterPort(bool isInternal, SelectHoverObject* parameterParent);

    Component *getParentEffect() override;

    bool canConnect(const ConnectionPort* other) const override;

    Component *getDragLineParent() override;

    void mouseEnter(const MouseEvent &event) override;

    void mouseExit(const MouseEvent &event) override;

private:
    SelectHoverObject* parameterParent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterPort)
};

class AudioPort;
class InternalConnectionPort : public ConnectionPort
{
public:
    using Ptr = ReferenceCountedObjectPtr<InternalConnectionPort>;

    InternalConnectionPort(AudioPort* parent, bool isInput);

    Component *getDragLineParent() override;

    bool canConnect(const ConnectionPort* other) const override;
    AudioPort* audioPort;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalConnectionPort)
};

class AudioPort : public ConnectionPort
{
public:
    using Ptr = ReferenceCountedObjectPtr<AudioPort>;

    Component *getDragLineParent() override;

    explicit AudioPort(bool isInput);

    bool hitTest(int x, int y) override {
        return hoverBox.contains(x,y);
    }

    const InternalConnectionPort::Ptr getInternalConnectionPort() const { return internalPort; }

    AudioProcessor::Bus* bus;
    InternalConnectionPort::Ptr internalPort;

    bool canConnect(const ConnectionPort* other) const override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPort)
};
