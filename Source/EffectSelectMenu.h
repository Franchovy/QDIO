/*
  ==============================================================================

    EffectSelectMenu.h
    Created: 28 Apr 2020 1:20:01pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Effect.h"


class EffectSelectMenu : public ComboBox
{
public:
    EffectSelectMenu();
    void mouseDown(const MouseEvent &event) override;
};


// delete dis
class EffectSelectComponent : public PopupMenu::CustomComponent
{
public:
    EffectSelectComponent();

    void mouseDown(const MouseEvent &event) override;

    void getIdealSize(int &idealWidth, int &idealHeight) override;

    void paint(Graphics &g) override;

    Label label;
    Image image;
};
