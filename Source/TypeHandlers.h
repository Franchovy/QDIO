/*
  ==============================================================================

    TypeHandlers.h
    Created: 13 Apr 2020 4:11:10pm
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "IDs"


using TypeHandler = ComponentBuilder::TypeHandler;

class EffectSceneTypeHandler : TypeHandler
{
    EffectSceneTypeHandler() : TypeHandler(IDs::EFFECTSCENE_ID) {

    }

    Component *addNewComponentFromState(const ValueTree &state, Component *parent) override;

    void updateComponentFromState(Component *component, const ValueTree &state) override;
};