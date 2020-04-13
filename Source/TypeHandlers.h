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

#include "EffectScene.h"


using TypeHandler = ComponentBuilder::TypeHandler;

/*class EffectSceneTypeHandler : public TypeHandler
{
public:
    EffectSceneTypeHandler() : TypeHandler(EFFECTSCENE_ID) {

    }

    Component *addNewComponentFromState(const ValueTree &state, Component *parent) override;

    void updateComponentFromState(Component *component, const ValueTree &state) override;
};*/

class EffectTypeHandler : public TypeHandler
{
public:
    EffectTypeHandler() : TypeHandler(EFFECT_ID) {

    }

    Component *addNewComponentFromState(const ValueTree &state, Component *parent) override;

    void updateComponentFromState(Component *component, const ValueTree &state) override;
};

class ConnectionTypeHandler : public TypeHandler
{
public:
    ConnectionTypeHandler() : TypeHandler(CONNECTION_ID) {

    }

    Component *addNewComponentFromState(const ValueTree &state, Component *parent) override;

    void updateComponentFromState(Component *component, const ValueTree &state) override;
};

