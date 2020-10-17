/*
  ==============================================================================

    ConnectionPort.h
    Created: 15 Oct 2020 11:19:35am
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SceneComponent.h"

class ConnectionPort : public SceneComponent
{
public:
    ConnectionPort();

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
};