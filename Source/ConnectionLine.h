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


class ConnectionLine : public SelectHoverObject, public ComponentListener
{
public:
    using Ptr = ReferenceCountedObjectPtr<ConnectionLine>;

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;

/*
    void parentHierarchyChanged() override;
    void visibilityChanged() override;
*/

    ConnectionLine();
    ~ConnectionLine() override;

    bool setPort(ConnectionPort* port);
    void unsetPort(ConnectionPort* port);

    void resized() override;

    void paint(Graphics &g) override;
    bool hitTest(int x, int y) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;

    bool canConnect(const ConnectionPort* port) const;

    bool canDragInto(const SelectHoverObject *other, bool isRightClickDrag = false) const override;
    bool canDragHover(const SelectHoverObject *other, bool isRightClickDrag = false) const override;

    ConnectionPort* getOtherPort(const ConnectionPort::Ptr& port);
    ConnectionPort* getInPort() const {
        return inPort.get();
    }
    ConnectionPort* getOutPort() const {
        return outPort.get();
    }

    void reconnect(ConnectionPort *newInPort, ConnectionPort *newOutPort);

    bool connect();
    void disconnect(ConnectionPort* port = nullptr);

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

    bool isConnected() const;

    enum Type {
        audio = 0,
        parameter = 1
    } type;

private:
    AudioProcessorGraph::Connection audioConnection;

    Point<int> inPos, outPos;
    Line<int> line;

    bool connected = false;

    ConnectionPort::Ptr inPort = nullptr;
    ConnectionPort::Ptr outPort = nullptr;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionLine)

};

