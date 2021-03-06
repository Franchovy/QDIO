/*
  ==============================================================================

    This code is based on the contents of the book: "Audio Effects: Theory,
    Implementation and Application" by Joshua D. Reiss and Andrew P. McPherson.

    Code by Juan Gil <https://juangil.com/>.
    Copyright (C) 2017-2019 Juan Gil.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Source/BaseEffects.h"

//==============================================================================

class FlangerAudioProcessor : public BaseEffect
{
public:
    //==============================================================================

    FlangerAudioProcessor();
    ~FlangerAudioProcessor() override;

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;


    //==============================================================================

    StringArray waveformItemsUI = {
        "Sine",
        "Triangle",
        "Sawtooth (rising)",
        "Sawtooth (falling)"
    };

    enum waveformIndex {
        waveformSine = 0,
        waveformTriangle,
        waveformSawtooth,
        waveformInverseSawtooth,
    };

    //======================================

    StringArray interpolationItemsUI = {
        "None",
        "Linear",
        "Cubic"
    };

    enum interpolationIndex {
        interpolationNearestNeighbour = 0,
        interpolationLinear,
        interpolationCubic,
    };

    //======================================

    AudioSampleBuffer delayBuffer;
    int delayBufferSamples;
    int delayBufferChannels;
    int delayWritePosition;

    float lfoPhase;
    float inverseSampleRate;
    float twoPi;

    float lfo (float phase, int waveform);

    //======================================

    AudioParameterFloat* paramDelay;
    AudioParameterFloat* paramWidth;
    AudioParameterFloat* paramDepth;
    AudioParameterFloat* paramFeedback;
    AudioParameterBool* paramInverted;
    AudioParameterFloat* paramFrequency;
    AudioParameterChoice* paramWaveform;
    AudioParameterChoice* paramInterpolation;
    AudioParameterBool* paramStereo;

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlangerAudioProcessor)
};
