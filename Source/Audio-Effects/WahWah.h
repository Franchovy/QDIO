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
#include "../BaseEffects.h"

//==============================================================================

class WahWahAudioProcessor : public BaseEffect
{
public:
    //==============================================================================

    WahWahAudioProcessor();
    ~WahWahAudioProcessor();

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================

    StringArray modeItemsUI = {
        "Manual",
        "Automatic"
    };

    enum modeIndex {
        modeManual = 0,
        modeAutomatic,
    };

    StringArray filterTypeItemsUI = {
        "Resonant Low-pass",
        "Band-pass",
        "Peaking/Notch"
    };

    enum filterTypeIndex {
        filterTypeResonantLowPass = 0,
        filterTypeBandPass,
        filterTypePeakingNotch,
    };

    //======================================

    class Filter : public IIRFilter
    {
    public:
        void updateCoefficients (const double discreteFrequency,
                                 const double qFactor,
                                 const double gain,
                                 const int filterType) noexcept
        {
            jassert (discreteFrequency > 0);
            jassert (qFactor > 0);

            double bandwidth = jmin (discreteFrequency / qFactor, M_PI * 0.99);
            double two_cos_wc = -2.0 * cos (discreteFrequency);
            double tan_half_bw = tan (bandwidth / 2.0);
            double tan_half_wc = tan (discreteFrequency / 2.0);
            double tan_half_wc_2 = tan_half_wc * tan_half_wc;
            double sqrt_gain = sqrt (gain);

            switch (filterType) {
                case filterTypeResonantLowPass: {
                    coefficients = IIRCoefficients (/* b0 */ tan_half_wc_2,
                                                    /* b1 */ tan_half_wc_2 * 2,
                                                    /* b2 */ tan_half_wc_2,
                                                    /* a0 */ tan_half_wc_2 + tan_half_wc / gain + 1.0,
                                                    /* a1 */ 2 * tan_half_wc_2 - 2.0,
                                                    /* a2 */ tan_half_wc_2 - tan_half_wc / gain + 1.0);
                    break;
                }
                case filterTypeBandPass: {
                    coefficients = IIRCoefficients (/* b0 */ tan_half_bw,
                                                    /* b1 */ 0.0,
                                                    /* b2 */ -tan_half_bw,
                                                    /* a0 */ 1.0 + tan_half_bw,
                                                    /* a1 */ two_cos_wc,
                                                    /* a2 */ 1.0 - tan_half_bw);
                    break;
                }
                case filterTypePeakingNotch: {
                    coefficients = IIRCoefficients (/* b0 */ sqrt_gain + gain * tan_half_bw,
                                                    /* b1 */ sqrt_gain * two_cos_wc,
                                                    /* b2 */ sqrt_gain - gain * tan_half_bw,
                                                    /* a0 */ sqrt_gain + tan_half_bw,
                                                    /* a1 */ sqrt_gain * two_cos_wc,
                                                    /* a2 */ sqrt_gain - tan_half_bw);
                    break;
                }
            }

            setCoefficients (coefficients);
        }
    };

    OwnedArray<Filter> filters;
    void updateFilters();

    float centreFrequency;
    float lfoPhase;
    float inverseSampleRate;
    float twoPi;

    Array<float> envelopes;
    float inverseE;
    float calculateAttackOrRelease (float value);

    //======================================

    AudioParameterChoice* paramMode;
    AudioParameterFloat* paramMix;
    AudioParameterFloat* paramFrequency; //log
    AudioParameterFloat* paramQfactor;
    AudioParameterFloat* paramGain;
    AudioParameterChoice* paramFilterType;
    AudioParameterFloat* paramLFOfrequency;
    AudioParameterFloat* paramMixLFOandEnvelope;
    AudioParameterFloat* paramEnvelopeAttack;
    AudioParameterFloat* paramEnvelopeRelease;

private:
    std::atomic<float> freqValue;
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WahWahAudioProcessor)
};
