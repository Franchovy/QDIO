/*
  ==============================================================================

    EffectManager.cpp
    Created: 21 Jul 2020 1:11:30pm
    Author:  maxime

  ==============================================================================
*/

#include "EffectManager.h"

EffectManager* EffectManager::instance = nullptr;

EffectManager::EffectManager() {
    instance = this;
}

EffectManager *EffectManager::getInstance() {
    jassert(instance != nullptr);
    return instance;
}
