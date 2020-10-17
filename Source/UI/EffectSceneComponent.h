/*
  ==============================================================================

    EffectSceneComponent.h
    Created: 15 Oct 2020 11:18:51am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include "SceneComponent.h"
#include "ConnectionContainer.h"
#include "EffectComponent.h"

class EffectSceneComponent : public SceneComponent, public ConnectionContainer
{
public:
    EffectSceneComponent();

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

private:
    //=============================================================================
    // Test stuff
    SceneComponent childComponent;
    SceneComponent parentComponent;

    EffectComponent effect;
    EffectComponent effect2;
};