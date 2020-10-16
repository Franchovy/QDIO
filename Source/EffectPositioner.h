/*
  ==============================================================================

    EffectPositioner.h
    Created: 6 Jul 2020 10:52:30am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Effect;
class EffectBase;
class ConnectionLine;
class ConnectionPort_old;


/**
 * EffectPositioner is a "UI companion" taking care of UI shortcuts and
 * smoothing out the experience for the user. It also takes care of structural changes
 * by listening to UI and sends messages to the UItoLogicMessager when needed.
 */
class EffectPositioner
{
public:
    EffectPositioner();

    static EffectPositioner* getInstance();
    static void setPositionerRunning(bool runState);

    //=================================================================================
    // Listener methods to be called by UI System

    void effectResized(Effect* effect);
    void effectMoved(Effect* effect);
    void effectCreated(Effect* effect);
    void effectDeleted(Effect* effect);
    void effectParentReassigned(Effect* effect, EffectBase* parent);

    void setScene(EffectBase* scene);
private:

    //=================================================================================
    // Shortcut actions

    void autoPlace(Effect* effect);
    void makeRoomForEffect(Effect* effect);

    void fitEffects(Effect* leftEffect, Effect* rightEffect, bool expandRightwards);

    void refitChildren(Effect* parentEffect);
    void moveEffect(Effect* effect, int distance, bool rightWard);
    void removeEffectConnections(Effect* effect);
    void insertEffect(Effect* effect, ConnectionLine* line);
    void swapEffects(Effect* effectDragged, Effect* effectToMove);
    void mergeConnection(ConnectionLine* line1, ConnectionLine* line2);
    void extendConnection(ConnectionLine* lineToExtend, Effect* parentToExtendThrough);
    void shortenConnection(ConnectionLine *interiorLine, ConnectionLine *exteriorLine);


    //=================================================================================
    // Positioning functions
    int getFittedDistance(const Effect* leftEffect, const Effect* rightEffect) const;

    // Connectivity Convenience functions
    ConnectionLine* getConnectionToPort(ConnectionPort_old* port) const;

    // Geometric Convenience functions
    Point<int> getEffectCenter(Effect* effect) const;
    int getDistanceFromLineExtended(Line<int>line, Point<int> point) const;

    //=================================================================================

    static EffectPositioner* instance;

    EffectBase* scene = nullptr;

    bool positionerRunning = false;
    bool movingOp = false;

    // Magic numbers
    int minDistanceBetweenEffects = 40;
};
