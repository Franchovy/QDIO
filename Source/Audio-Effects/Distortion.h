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

class DistortionAudioProcessor : public BaseEffect, public Timer
{
public:
    //==============================================================================

    DistortionAudioProcessor();
    ~DistortionAudioProcessor();

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    void timerCallback() override;

    //==============================================================================

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    //==============================================================================


    StringArray distortionTypeItemsUI = {
        "Hard clipping",
        "Soft clipping",
        "Exponential",
        "Full-wave rectifier",
        "Half-wave rectifier"
    };

    enum distortionTypeIndex {
        distortionTypeHardClipping = 0,
        distortionTypeSoftClipping,
        distortionTypeExponential,
        distortionTypeFullWaveRectifier,
        distortionTypeHalfWaveRectifier,
    };

    //======================================

    class Filter : public IIRFilter
    {
    public:
        void updateCoefficients (const double discreteFrequency,
                                 const double gain) noexcept
        {
            jassert (discreteFrequency > 0);

            double tan_half_wc = tan (discreteFrequency / 2.0);
            double sqrt_gain = sqrt (gain);

            coefficients = IIRCoefficients (/* b0 */ sqrt_gain * tan_half_wc + gain,
                                            /* b1 */ sqrt_gain * tan_half_wc - gain,
                                            /* b2 */ 0.0,
                                            /* a0 */ sqrt_gain * tan_half_wc + 1.0,
                                            /* a1 */ sqrt_gain * tan_half_wc - 1.0,
                                            /* a2 */ 0.0);

            setCoefficients (coefficients);
        }
    };

    OwnedArray<Filter> filters;
    void updateFilters();

    //======================================

    AudioParameterChoice* paramDistortionType;
    AudioParameterFloat* paramInputGain;
    AudioParameterFloat* paramOutputGain;
    AudioParameterFloat* paramTone;

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionAudioProcessor)
};