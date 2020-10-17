/*
  ==============================================================================

    ConnectionPort.cpp
    Created: 15 Oct 2020 11:19:35am
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionPort.h"

ConnectionPort::ConnectionPort() {

}

void ConnectionPort::mouseDown(const MouseEvent &event) {
    getParentComponent()->mouseDown(event);
    SceneComponent::mouseDown(event);
}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    getParentComponent()->mouseDrag(event);
    SceneComponent::mouseDrag(event);
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    getParentComponent()->mouseUp(event);
    SceneComponent::mouseUp(event);
}
