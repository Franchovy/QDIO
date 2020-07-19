/*
  ==============================================================================

    UItoLogicMessager.h
    Created: 19 Jul 2020 10:49:24am
    Author:  maxime

  ==============================================================================
*/

#pragma once

class UItoLogicMessager
{
public:
    UItoLogicMessager();
    static UItoLogicMessager* getInstance();


private:
    static UItoLogicMessager* instance;
};

