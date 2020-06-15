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
#include "STFT.h"

//==============================================================================

class RobotizationWhisperizationAudioProcessor : public BaseEffect
{
public:
    //==============================================================================

    RobotizationWhisperizationAudioProcessor();
    ~RobotizationWhisperizationAudioProcessor();

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================


    StringArray effectItemsUI = {
        "Pass-through",
        "Robotization",
        "Whisperization"
    };

    enum effectIndex {
        effectPassThrough = 0,
        effectRobotization,
        effectWhisperization,
    };

    //======================================

    StringArray fftSizeItemsUI = {
        "32",
        "64",
        "128",
        "256",
        "512",
        "1024",
        "2048",
        "4096",
        "8192"
    };

    enum fftSizeIndex {
        fftSize32 = 0,
        fftSize64,
        fftSize128,
        fftSize256,
        fftSize512,
        fftSize1024,
        fftSize2048,
        fftSize4096,
        fftSize8192,
    };

    //======================================

    StringArray hopSizeItemsUI = {
        "1/2 Window",
        "1/4 Window",
        "1/8 Window",
    };

    enum hopSizeIndex {
        hopSize2 = 0,
        hopSize4,
        hopSize8,
    };

    //======================================

    StringArray windowTypeItemsUI = {
        "Rectangular",
        "Bartlett",
        "Hann",
        "Hamming",
    };

    //======================================

    class RobotizationWhisperization : public STFT
    {
    public:
        RobotizationWhisperization (RobotizationWhisperizationAudioProcessor& p) : parent (p)
        {
        }

    private:
        void modification() override
        {
            fft->perform (timeDomainBuffer, frequencyDomainBuffer, false);

            auto size = (int)(float)(1 << ((int)fftSize + 5));

            switch ((int)parent.paramEffect->getIndex()) {
                case effectPassThrough: {
                    // nothing
                    break;
                }
                case effectRobotization: {
                    for (int index = 0; index < size; ++index) {
                        float magnitude = abs (frequencyDomainBuffer[index]);
                        frequencyDomainBuffer[index].real (magnitude);
                        frequencyDomainBuffer[index].imag (0.0f);
                    }
                    break;
                }
                case effectWhisperization: {
                    for (int index = 0; index < size / 2 + 1; ++index) {
                        float magnitude = abs (frequencyDomainBuffer[index]);
                        float phase = 2.0f * M_PI * (float)rand() / (float)RAND_MAX;

                        frequencyDomainBuffer[index].real (magnitude * cosf (phase));
                        frequencyDomainBuffer[index].imag (magnitude * sinf (phase));
                        if (index > 0 && index < size / 2) {
                            frequencyDomainBuffer[size - index].real (magnitude * cosf (phase));
                            frequencyDomainBuffer[size - index].imag (magnitude * sinf (-phase));
                        }
                    }
                    break;
                }
            }

            fft->perform (frequencyDomainBuffer, timeDomainBuffer, true);
        }

        RobotizationWhisperizationAudioProcessor& parent;
    };

    //======================================

    CriticalSection lock;
    RobotizationWhisperization stft;

    //======================================

    AudioParameterChoice* paramEffect;
    AudioParameterChoice* paramFftSize;
    AudioParameterChoice* paramHopSize;
    AudioParameterChoice* paramWindowType;

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RobotizationWhisperizationAudioProcessor)
};
