/*
  ==============================================================================

    EffectSelectMenu.h
    Created: 28 Apr 2020 1:20:01pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class EffectSelectMenu : public ComboBox
{
public:
    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;

    void mouseUp(const MouseEvent &event) override;
};