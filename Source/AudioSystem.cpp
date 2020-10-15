/*
  ==============================================================================

    Audio.cpp
    Created: 6 Oct 2020 11:07:10am
    Author:  maxime

  ==============================================================================
*/

#include "AudioSystem.h"

AudioSystem::AudioSystem() {

    deviceManager.addAudioCallback(&processorPlayer);
    processorPlayer.setProcessor(&audioGraph);

}
