/*
  ==============================================================================

    EffectSelectMenu.h
    Created: 28 Apr 2020 1:20:01pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "EffectLoader.h"
#include "EffectScene.h"

class EffectSelectMenu : public ComboBox
{
public:
    EffectSelectMenu();

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectSelectMenu)
};