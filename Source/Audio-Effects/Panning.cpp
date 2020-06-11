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

#include "Panning.h"

//==============================================================================

PanningAudioProcessor::PanningAudioProcessor()
    : BaseEffect()
    , paramMethod (new AudioParameterChoice("Method", "method", methodItemsUI, methodItdIld))
    , paramPanning (new AudioParameterFloat("Panning", "panning",
            NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f))
{
    setLayout(1,1);

    addParameter(paramMethod);
    addParameter(paramPanning);
}

PanningAudioProcessor::~PanningAudioProcessor()
{
}

//==============================================================================

void PanningAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
//    paramMethod.reset (sampleRate, smoothTime);
//    paramPanning.reset (sampleRate, smoothTime);

    //======================================

    maximumDelayInSamples = (int)(1e-3f * (float)getSampleRate());
    delayLineL.setup (maximumDelayInSamples);
    delayLineR.setup (maximumDelayInSamples);
}

void PanningAudioProcessor::releaseResources()
{
}

void PanningAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    float currentPanning = *paramPanning;//.getNextValue();

    float* channelDataL = buffer.getWritePointer (0);
    float* channelDataR = buffer.getWritePointer (1);

    switch ((int)*paramMethod) {

        //======================================

        case methodPanoramaPrecedence: {
            // Panorama
            float theta = degreesToRadians (30.0f);
            float phi = -currentPanning * theta;
            float cos_theta = cosf (theta);
            float cos_phi = cosf (phi);
            float sin_theta = sinf (theta);
            float sin_phi = sinf (phi);
            float gainL = (cos_phi * sin_theta + sin_phi * cos_theta);
            float gainR = (cos_phi * sin_theta - sin_phi * cos_theta);
            float norm = 1.0f / sqrtf (gainL * gainL + gainR * gainR);

            // Precedence
            float delayFactor = (currentPanning + 1.0f) / 2.0f;
            float delayTimeL = (float)maximumDelayInSamples * (delayFactor);
            float delayTimeR = (float)maximumDelayInSamples * (1.0f - delayFactor);
            for (int sample = 0; sample < numSamples; ++sample) {
                const float in = channelDataL[sample];
                delayLineL.writeSample (in);
                delayLineR.writeSample (in);
                channelDataL[sample] = delayLineL.readSample (delayTimeL) * gainL * norm;
                channelDataR[sample] = delayLineR.readSample (delayTimeR) * gainR * norm;
            }
            break;
        }

        //======================================

        case methodItdIld: {
            float headRadius = 8.5e-2f;
            float speedOfSound = 340.0f;
            float headFactor = (float)getSampleRate() * headRadius / speedOfSound;

            // Interaural Time Difference (ITD)
            auto Td = [headFactor](const float angle){
                if (abs (angle) < (float)M_PI_2)
                    return headFactor * (1.0f - cosf (angle));
                else
                    return headFactor * (abs (angle) + 1.0f - (float)M_PI_2);
            };
            float theta = degreesToRadians (90.0f);
            float phi = currentPanning * theta;
            float currentDelayTimeL = Td (phi + (float)M_PI_2);
            float currentDelayTimeR = Td (phi - (float)M_PI_2);
            for (int sample = 0; sample < numSamples; ++sample) {
                const float in = channelDataL[sample];
                delayLineL.writeSample (in);
                delayLineR.writeSample (in);
                channelDataL[sample] = delayLineL.readSample (currentDelayTimeL);
                channelDataR[sample] = delayLineR.readSample (currentDelayTimeR);
            }

            // Interaural Level Difference (ILD)
            filterL.updateCoefficients ((double)phi + M_PI_2, (double)(headRadius / speedOfSound));
            filterR.updateCoefficients ((double)phi - M_PI_2, (double)(headRadius / speedOfSound));
            filterL.processSamples (channelDataL, numSamples);
            filterR.processSamples (channelDataR, numSamples);
            break;
        }
    }

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================





