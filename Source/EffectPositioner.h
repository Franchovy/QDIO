/*
  ==============================================================================

    EffectPositioner.h
    Created: 6 Jul 2020 10:52:30am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"

class EffectPositioner : public ComponentListener
{
public:
    EffectPositioner();

    static EffectPositioner* getInstance();

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;
    void componentParentHierarchyChanged(Component &component) override;

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