/*
  ==============================================================================

    TypeHandlers.cpp
    Created: 13 Apr 2020 4:11:10pm
    Author:  maxime

  ==============================================================================
*/

#include "TypeHandlers.h"

/*Component *EffectSceneTypeHandler::addNewComponentFromState(const ValueTree &state, Component *parent) {
    return new EffectScene();
}

void EffectSceneTypeHandler::updateComponentFromState(Component *component, const ValueTree &state) {

}*/

Component *EffectTypeHandler::addNewComponentFromState(const ValueTree &state, Component *parent) {
    return nullptr;
}

void EffectTypeHandler::updateComponentFromState(Component *component, const ValueTree &state) {

}

Component *ConnectionTypeHandler::addNewComponentFromState(const ValueTree &state, Component *parent) {
    return nullptr;
}

void ConnectionTypeHandler::updateComponentFromState(Component *component, const ValueTree &state) {

}
