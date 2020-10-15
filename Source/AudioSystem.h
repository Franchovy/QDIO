/*
  ==============================================================================

    Audio.h
    Created: 6 Oct 2020 11:07:10am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
 * This is the main class running all audio bits for the app.
 * Interfaces with the Effect system through public methods
 */
class AudioSystem
{
public:
    AudioSystem();

    //===========================================================
    // ConnectionGraph

    //===========================================================
    // Devices, channels

    AudioDeviceManager* getDeviceManager() {return &deviceManager;}

private:
    AudioProcessorGraph audioGraph;
    AudioDeviceManager deviceManager;
    AudioProcessorPlayer processorPlayer;
};