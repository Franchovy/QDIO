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

class PhaserAudioProcessor : public BaseEffect
{
public:
    //==============================================================================

    PhaserAudioProcessor();
    ~PhaserAudioProcessor();

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;


    //==============================================================================

    StringArray waveformItemsUI = {
        "Sine",
        "Triangle",
        "Square",
        "Sawtooth"
    };

    enum waveformIndex {
        waveformSine = 0,
        waveformTriangle,
        waveformSquare,
        waveformSawtooth,
    };

    //======================================

    class Filter : public IIRFilter
    {
    public:
        void updateCoefficients (const double discreteFrequency) noexcept
        {
            jassert (discreteFrequency > 0);

            double wc = jmin (discreteFrequency, M_PI * 0.99);
            double tan_half_wc = tan (wc / 2.0);

            coefficients = IIRCoefficients (/* b0 */ tan_half_wc - 1.0,
                                            /* b1 */ tan_half_wc + 1.0,
                                            /* b2 */ 0.0,
                                            /* a0 */ tan_half_wc + 1.0,
                                            /* a1 */ tan_half_wc - 1.0,
                                            /* a2 */ 0.0);

            setCoefficients (coefficients);
        }
    };

    OwnedArray<Filter> filters;
    Array<float> filteredOutputs;
    void updateFilters (double centreFrequency);
    int numFiltersPerChannel;
    unsigned int sampleCountToUpdateFilters;
    unsigned int updateFiltersInterval;

    float lfoPhase;
    float inverseSampleRate;
    float twoPi;

    float lfo (float phase, int waveform);

    //======================================

    AudioParameterFloat* paramDepth;
    AudioParameterFloat* paramFeedback;
    AudioParameterChoice* paramNumFilters;
    AudioParameterFloat* paramMinFrequency; // log
    AudioParameterFloat* paramSweepWidth; // log
    AudioParameterFloat* paramLFOfrequency;
    AudioParameterChoice* paramLFOwaveform;
    AudioParameterBool* paramStereo;

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserAudioProcessor)
};
