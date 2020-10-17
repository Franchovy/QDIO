/*
  ==============================================================================

    ConnectionContainer.cpp
    Created: 15 Oct 2020 11:16:43am
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionContainer.h"

void ConnectionContainer::addConnection(ConnectionPort *port1, ConnectionPort *port2) {


}

void ConnectionContainer::startConnectionDrag(ConnectionPort *port1, Point<float> mousePos) {
    std::cout << "Drag start: " << mousePos.toString() << newLine;
}

void ConnectionContainer::connectionDrag(Point<float> mousePos) {
    std::cout << "Dragging: " << mousePos.toString() << newLine;
}

void ConnectionContainer::endConnectionDrag(Point<float> mousePos) {
    std::cout << "Drag end: " << mousePos.toString() << newLine;
}
