/*
  ==============================================================================

    ConnectionLine.h
    Created: 24 Apr 2020 10:45:08am
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>
#include "EffectGui.h"
#include "Ports.h"


#pragma once


class LineComponent : public GuiObject
{
public:
    LineComponent() = default;

    void paint(Graphics &g) override;

    /**
     * Updates dragLineTree connection property if connection is successful
     * @param port2
     */
    void convert(ConnectionPort* port2);

    void startDrag(ConnectionPort* p1, const MouseEvent& event);
    void drag(const MouseEvent& event);
    void release(ConnectionPort* p2);

    ConnectionPort* getPort1();

private:
    ConnectionPort* port1 = nullptr;

    Line<int> line;
    Point<int> p1, p2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LineComponent)
};



class ConnectionLine : public SelectHoverObject, public ComponentListener
{
public:
    using Ptr = ReferenceCountedObjectPtr<ConnectionLine>;

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;

    void componentParentHierarchyChanged(Component &component) override;

    ConnectionLine(ConnectionPort& p1, ConnectionPort& p2);

    ~ConnectionLine();

    void paint(Graphics &g) override;

    bool hitTest(int x, int y) override;

    ConnectionPort::Ptr getOtherPort(const ConnectionPort::Ptr& port);

    ConnectionPort::Ptr getInPort() {
        return inPort;
    }

    ConnectionPort::Ptr getInPort() const {
        return inPort;
    }

    ConnectionPort::Ptr getOutPort() {
        return outPort;
    }

    ConnectionPort::Ptr getOutPort() const {
        return outPort;
    }

    void setInPort(ConnectionPort::Ptr newInPort) {
        inPort = newInPort;
    }

    void setOutPort(ConnectionPort::Ptr newOutPort) {
        outPort = newOutPort;
    }

    void setAudioConnection(AudioProcessorGraph::Connection connection) {
        audioConnection = connection;
    }

    AudioProcessorGraph::Connection getAudioConnection() const {
        return audioConnection;
    }

    struct IDs {
        static const Identifier CONNECTION_ID;
        static const Identifier InPort;
        static const Identifier OutPort;
        static const Identifier ConnectionLineObject;
        static const Identifier AudioConnection;
    };

    bool canDragInto(SelectHoverObject *other) override;

private:
    Line<int> line;
    AudioProcessorGraph::Connection audioConnection;

    ConnectionPort::Ptr inPort;
    ConnectionPort::Ptr outPort;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionLine)
};

