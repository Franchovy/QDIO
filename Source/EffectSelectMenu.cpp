/*
  ==============================================================================

    EffectSelectMenu.cpp
    Created: 28 Apr 2020 1:20:01pm
    Author:  maxime

  ==============================================================================
*/

#include "EffectSelectMenu.h"


EffectSelectMenu::EffectSelectMenu() : ComboBox()
{
    Image image = ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);

    getRootMenu()->addItem(1, "poop", true, false, image);
}

void EffectSelectMenu::mouseDown(const MouseEvent &event) {
    ComboBox::mouseDown(event);
}



EffectSelectComponent::EffectSelectComponent()
    : PopupMenu::CustomComponent()
    , label("name", "test")
{
    label.setBounds(0, 0, 100, 50);
    label.setColour(Label::textColourId, Colours::blue);
    addAndMakeVisible(label);
}


void EffectSelectComponent::mouseDown(const MouseEvent &event) {
    Component::mouseDown(event);
}

void EffectSelectComponent::getIdealSize(int &idealWidth, int &idealHeight) {
    idealWidth = 400;
    idealHeight = 200;
}

void EffectSelectComponent::paint(Graphics &g) {
    g.setColour(Colours::hotpink);
    g.fillRect(getBounds());
    Component::paint(g);
}

