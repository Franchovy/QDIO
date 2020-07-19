/*
  ==============================================================================

    UItoLogicMessager.cpp
    Created: 19 Jul 2020 10:49:24am
    Author:  maxime

  ==============================================================================
*/

#include "UItoLogicMessager.h"

UItoLogicMessager* UItoLogicMessager::instance = nullptr;

UItoLogicMessager::UItoLogicMessager() {
    instance = this;
}

UItoLogicMessager *UItoLogicMessager::getInstance() {
    return instance;
}
