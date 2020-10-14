/*
  ==============================================================================

    SceneComponent.h
    Created: 14 Oct 2020 10:34:39am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SceneComponent : public Component, public ReferenceCountedObject
{
public:
    SceneComponent();
    void paint(Graphics &g) override;



    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

private:
    const int SELECT_MAX_DISTANCE = 5;

    static ReferenceCountedArray<SceneComponent> selectedComponents;

    bool selected = false;
    bool hovered = false;
};