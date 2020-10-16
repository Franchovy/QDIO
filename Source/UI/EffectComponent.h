/*
  ==============================================================================

    EffectComponent.h
    Created: 15 Oct 2020 11:18:35am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SceneComponent.h"
#include "EffectConnectable.h"
#include "ConnectionContainer.h"

class EffectComponent : public SceneComponent, public EffectConnectable, public ConnectionContainer
{
public:
    EffectComponent();

    void setNumPorts(int numIn, int numOut) override;

    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;

    void mouseUp(const MouseEvent &event) override;

private:


};