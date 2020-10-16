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

    ConnectionLine();
    ~ConnectionLine() override;

    bool setPort(ConnectionPort_old* port);
    void unsetPort(ConnectionPort_old* port);

    void resized() override;

    void paint(Graphics &g) override;
    bool hitTest(int x, int y) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;

    bool canConnect(const ConnectionPort_old* port) const;

    bool canDragInto(const SelectHoverObject *other, bool isRightClickDrag = false) const override;
    bool canDragHover(const SelectHoverObject *other, bool isRightClickDrag = false) const override;

    ConnectionPort_old* getOtherPort(const ConnectionPort_old::Ptr& port);
    ConnectionPort_old* getInPort() const {
        return inPort.get();
    }
    ConnectionPort_old* getOutPort() const {
        return outPort.get();
    }

    void reconnect(ConnectionPort_old *newInPort, ConnectionPort_old *newOutPort);

    bool connect();
    void disconnect(ConnectionPort_old* port = nullptr);

    void setAudioConnection(AudioProcessorGraph::Connection connection) {
        audioConnection = connection;
    }

    AudioProcessorGraph::Connection getAudioConnection() const {
        return audioConnection;
    }

    Line<int> getLine();

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

    ConnectionPort_old::Ptr inPort = nullptr;
    ConnectionPort_old::Ptr outPort = nullptr;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionLine)

};

