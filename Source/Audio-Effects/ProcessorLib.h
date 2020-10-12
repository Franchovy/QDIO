/*
  ==============================================================================

    ProcessorLib.h
    Created: 12 Oct 2020 9:46:10am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include "../BaseEffects.h"
#include "Chorus.h"
#include "Delay.h"
#include "CompressorExpander.h"
#include "ChannelSplitter.h"
#include "Distortion.h"
#include "Flanger.h"
#include "Gain.h"
#include "Panning.h"
#include "ParametricEQ.h"
#include "Phaser.h"
#include "PitchShift.h"
#include "Reverb.h"
#include "RingModulation.h"
#include "RobotizationWhisperization.h"
#include "Level.h"
#include "Tremolo.h"
#include "WahWah.h"
#include "IOEffects.h"
#include "Vibrato.h"
#include "Oscillator.h"
#include "ParameterMod.h"


class ProcessorLib {
public:
    static std::unique_ptr<AudioProcessor> createProcessor(int ID);
    static std::unique_ptr<AudioProcessor> createProcessor(String type);

    StringArray getProcessors();
    StringArray getParameterProcessors();
};