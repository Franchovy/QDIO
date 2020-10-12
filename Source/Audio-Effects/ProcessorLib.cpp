/*
  ==============================================================================

    ProcessorLib.cpp
    Created: 12 Oct 2020 9:46:10am
    Author:  maxime

  ==============================================================================
*/

#include "ProcessorLib.h"

std::unique_ptr<AudioProcessor> ProcessorLib::createProcessor(String type) {
    if (type == "Input") { return std::make_unique<IOEffect>(true); }
    if (type == "Output") { return std::make_unique<IOEffect>(false); }

    if (type == "Chorus") { return std::make_unique<ChorusAudioProcessor>(); }
    if (type == "Delay") { return std::make_unique<DelayAudioProcessor>(); }
    if (type == "CompressorExpander") { return std::make_unique<CompressorExpanderAudioProcessor>(); }
    if (type == "ChannelSplitter") { return std::make_unique<ChannelSplitterAudioProcessor>(); }
    if (type == "Distortion") { return std::make_unique<DistortionAudioProcessor>(); }
    if (type == "Flanger") { return std::make_unique<FlangerAudioProcessor>(); }
    if (type == "Gain") { return std::make_unique<GainAudioProcessor>(); }
    if (type == "Panning") { return std::make_unique<PanningAudioProcessor>(); }
    if (type == "ParametricEQ") { return std::make_unique<ParametricEQAudioProcessor>(); }
    if (type == "Phaser") { return std::make_unique<PhaserAudioProcessor>(); }
    if (type == "PitchShift") { return std::make_unique<PitchShiftAudioProcessor>(); }
    if (type == "Reverb") { return std::make_unique<ReverbAudioProcessor>(); }
    if (type == "RingModulation") { return std::make_unique<RingModulationAudioProcessor>(); }
    if (type == "RobotizationWhisperization") { return std::make_unique<RobotizationWhisperizationAudioProcessor>(); }
    if (type == "Tremolo") { return std::make_unique<TremoloAudioProcessor>(); }
    if (type == "WahWah") { return std::make_unique<WahWahAudioProcessor>(); }
    if (type == "Vibrato") { return std::make_unique<VibratoAudioProcessor>(); }

    if (type == "Level") { return std::make_unique<LevelAudioProcessor>(); }
    if (type == "Oscillator") { return std::make_unique<Oscillator>(); }
    if (type == "ParameterMod") { return std::make_unique<ParameterMod>(); }
    
    return std::unique_ptr<BaseEffect>();
}


std::unique_ptr<AudioProcessor> ProcessorLib::createProcessor(int ID) {
    StringArray processorNames = {
            "Empty Effect"
            , "Input"
            , "Output"
            , "Distortion"
            , "Delay"
            , "Reverb"
            , "Chorus"
            , "Compressor"
            , "Channel Splitter"
            , "Flanger"
            , "Gain"
            , "Panning"
            , "EQ"
            , "Phaser"
            , "RingModulation"
            , "PitchShift"
            , "Robotization"
            , "Level Detector"
            , "Tremolo"
            , "Wahwah"
            , "Vibrato"
            , "Parameter Oscillator"
            , "Parameter Mod"
    };
    String type = processorNames[ID];
    return createProcessor(type);
}
