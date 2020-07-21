/*
  ==============================================================================

    EffectManager.h
    Created: 21 Jul 2020 1:11:30pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class EffectManager
{
public:
    EffectManager();
    static EffectManager* getInstance();

private:
    static EffectManager* instance;
};