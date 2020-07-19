/*
  ==============================================================================

    UItoLogicMessager.h
    Created: 19 Jul 2020 10:49:24am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class UItoLogicMessager
{
public:
    UItoLogicMessager();

    static UItoLogicMessager* getInstance();

    // Create Effect functions
    void createEffect(int processorID); // Processor-based Effect
    void createEffect(); // Empty Meta-Effect
    void createEffect(ValueTree data); // Meta-Effect


private:
    static UItoLogicMessager* instance;
};

