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
class ConnectionPort;


/**
 * EffectListener class is used to pass messages from UI to the effect system.
 * It passes messages along to the EffectPositioner, which is UI-specific, and to the
 * EffectManager and ConnectionGraph, which are not UI-specific.
 */
class EffectListener
{
public:
    EffectListener();

    static EffectListener* getInstance();

    void effectDragged(Effect* effect);
    void effectResized(Effect* effect);

    void effectCreated(Effect* effect);
    void effectDeleted(Effect* effect);

    void effectMovedIntoParent(Effect* effect, EffectBase* newParent);

private:
    static EffectListener* instance;

    bool movingOp = false;

};

class EffectManager
{
public:
    EffectManager();
    static EffectManager* getInstance();


private:
    static EffectManager* instance;
};

class EffectPositioner
{
public:
    EffectPositioner();

    static EffectPositioner* getInstance();

    void repositionOnResize(Effect* effect);
    void repositionOnMove(Effect* effect);
    void repositionOnEnter(Effect* effect, EffectBase* parent);

    //todo void repositionEffect();

    int getFittedDistance(const Effect* leftEffect, const Effect* rightEffect) const;
    void moveEffect(Effect* effect, int distance, bool rightWard);

    void removeEffectConnections(Effect* effect);
    void insertEffect(Effect* effect, ConnectionLine* line);

    void swapEffects(Effect* effectDragged, Effect* effectToMove);

    void mergeConnection(ConnectionLine* line1, ConnectionLine* line2);
    void extendConnection(ConnectionLine* lineToExtend, Effect* parentToExtendThrough);
    void shortenConnection(ConnectionLine *interiorLine, ConnectionLine *exteriorLine);

    ConnectionLine* getConnectionToPort(ConnectionPort* port);

    // Convenience functions
    Point<int> getEffectCenter(Effect* effect);
    int getDistanceFromLineExtended(Line<int>line, Point<int> point);


private:
    static EffectPositioner* instance;

    bool movingOp = false;

    // Magic numbers
    int minDistanceBetweenEffects = 40;
};
