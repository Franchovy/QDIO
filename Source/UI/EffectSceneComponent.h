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

class EffectSceneComponent : public SceneComponent, public ConnectionContainer
{
public:
    EffectSceneComponent();

private:
    //=============================================================================
    // Test stuff
    SceneComponent childComponent;
    SceneComponent parentComponent;
};