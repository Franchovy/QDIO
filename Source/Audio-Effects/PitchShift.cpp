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

#include "PitchShift.h"

//==============================================================================

PitchShiftAudioProcessor::PitchShiftAudioProcessor():
    BaseEffect()
    , paramShift (new AudioParameterFloat("Shift", "shift",
            NormalisableRange<float>(-12.0f, 12.0f, 1.0f), 0.0f, " Semitone(s)"))
                  //[this](float value){ return powf (2.0f, value / 12.0f); })
    , paramFftSize (new AudioParameterChoice("FFT size", "fftsize", fftSizeItemsUI, fftSize512))
                    /*[this](float value){
                        const ScopedLock sl (lock);
                        value = (float)(1 << ((int)value + 5));
                        paramFftSize.setCurrentAndTargetValue (value);
                        updateFftSize();
                        updateHopSize();
                        updateAnalysisWindow();
                        updateWindowScaleFactor();
                        return value;
                    })*/
    , paramHopSize (new AudioParameterChoice("Hop size", "hopsize", hopSizeItemsUI, hopSize8))
                    /*[this](float value){
                        const ScopedLock sl (lock);
                        value = (float)(1 << ((int)value + 1));
                        paramHopSize.setCurrentAndTargetValue (value);
                        updateFftSize();
                        updateHopSize();
                        updateAnalysisWindow();
                        updateWindowScaleFactor();
                        return value;
                    })*/
    , paramWindowType (new AudioParameterChoice("Window type", "windowtype", windowTypeItemsUI, windowTypeHann))
                       /*[this](float value){
                           const ScopedLock sl (lock);
                           paramWindowType.setCurrentAndTargetValue (value);
                           updateFftSize();
                           updateHopSize();
                           updateAnalysisWindow();
                           updateWindowScaleFactor();
                           return value;
                       })*/
{
    setLayout(1,1);
    addParameter(paramShift);
    addParameter(paramFftSize);
    addParameter(paramHopSize);
    addParameter(paramWindowType);
}

PitchShiftAudioProcessor::~PitchShiftAudioProcessor()
{
}

//==============================================================================

void PitchShiftAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
/*
    paramShift.reset (sampleRate, smoothTime);
    paramFftSize.reset (sampleRate, smoothTime);
    paramHopSize.reset (sampleRate, smoothTime);
    paramWindowType.reset (sampleRate, smoothTime);
*/

    //======================================

    needToResetPhases = true;
}

void PitchShiftAudioProcessor::releaseResources()
{
}

void PitchShiftAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const ScopedLock sl (lock);

    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    int currentInputBufferWritePosition;
    int currentOutputBufferWritePosition;
    int currentOutputBufferReadPosition;
    int currentSamplesSinceLastFFT;

    float shift = *paramShift;
    float ratio = roundf (shift * (float)hopSize) / (float)hopSize;
    int resampledLength = floorf ((float)fftSize / ratio);
    HeapBlock<float> resampledOutput (resampledLength, true);
    HeapBlock<float> synthesisWindow (resampledLength, true);
    updateWindow (synthesisWindow, resampledLength);

    for (int channel = 0; channel < numInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer (channel);

        currentInputBufferWritePosition = inputBufferWritePosition;
        currentOutputBufferWritePosition = outputBufferWritePosition;
        currentOutputBufferReadPosition = outputBufferReadPosition;
        currentSamplesSinceLastFFT = samplesSinceLastFFT;

        for (int sample = 0; sample < numSamples; ++sample) {

            //======================================

            const float in = channelData[sample];
            channelData[sample] = outputBuffer.getSample (channel, currentOutputBufferReadPosition);

            //======================================

            outputBuffer.setSample (channel, currentOutputBufferReadPosition, 0.0f);
            if (++currentOutputBufferReadPosition >= outputBufferLength)
                currentOutputBufferReadPosition = 0;

            //======================================

            inputBuffer.setSample (channel, currentInputBufferWritePosition, in);
            if (++currentInputBufferWritePosition >= inputBufferLength)
                currentInputBufferWritePosition = 0;

            //======================================

            if (++currentSamplesSinceLastFFT >= hopSize) {
                currentSamplesSinceLastFFT = 0;

                //======================================

                int inputBufferIndex = currentInputBufferWritePosition;
                for (int index = 0; index < fftSize; ++index) {
                    fftTimeDomain[index].real (sqrtf (fftWindow[index]) * inputBuffer.getSample (channel, inputBufferIndex));
                    fftTimeDomain[index].imag (0.0f);

                    if (++inputBufferIndex >= inputBufferLength)
                        inputBufferIndex = 0;
                }

                //======================================

                fft->perform (fftTimeDomain, fftFrequencyDomain, false);

                if (*paramShift)
                    needToResetPhases = true;
                if (shift == *paramShift && needToResetPhases) {
                    inputPhase.clear();
                    outputPhase.clear();
                    needToResetPhases = false;
                }

                for (int index = 0; index < fftSize; ++index) {
                    float magnitude = abs (fftFrequencyDomain[index]);
                    float phase = arg (fftFrequencyDomain[index]);

                    float phaseDeviation = phase - inputPhase.getSample (channel, index) - omega[index] * (float)hopSize;
                    float deltaPhi = omega[index] * hopSize + princArg (phaseDeviation);
                    float newPhase = princArg (outputPhase.getSample (channel, index) + deltaPhi * ratio);

                    inputPhase.setSample (channel, index, phase);
                    outputPhase.setSample (channel, index, newPhase);
                    fftFrequencyDomain[index] = std::polar (magnitude, newPhase);
                }

                fft->perform (fftFrequencyDomain, fftTimeDomain, true);

                for (int index = 0; index < resampledLength; ++index) {
                    float x = (float)index * (float)fftSize / (float)resampledLength;
                    int ix = (int)floorf (x);
                    float dx = x - (float)ix;

                    float sample1 = fftTimeDomain[ix].real();
                    float sample2 = fftTimeDomain[(ix + 1) % fftSize].real();
                    resampledOutput[index] = sample1 + dx * (sample2 - sample1);
                    resampledOutput[index] *= sqrtf (synthesisWindow[index]);
                }

                //======================================

                int outputBufferIndex = currentOutputBufferWritePosition;
                for (int index = 0; index < resampledLength; ++index) {
                    float out = outputBuffer.getSample (channel, outputBufferIndex);
                    out += resampledOutput[index] * windowScaleFactor;
                    outputBuffer.setSample (channel, outputBufferIndex, out);

                    if (++outputBufferIndex >= outputBufferLength)
                        outputBufferIndex = 0;
                }

                //======================================

                currentOutputBufferWritePosition += hopSize;
                if (currentOutputBufferWritePosition >= outputBufferLength)
                    currentOutputBufferWritePosition = 0;
            }

            //======================================
        }
    }

    inputBufferWritePosition = currentInputBufferWritePosition;
    outputBufferWritePosition = currentOutputBufferWritePosition;
    outputBufferReadPosition = currentOutputBufferReadPosition;
    samplesSinceLastFFT = currentSamplesSinceLastFFT;

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================

void PitchShiftAudioProcessor::updateFftSize()
{
    fftSize = (int) *paramFftSize;
    fft = std::make_unique<dsp::FFT>(log2 (fftSize));

    inputBufferLength = fftSize;
    inputBufferWritePosition = 0;
    inputBuffer.clear();
    inputBuffer.setSize (getTotalNumInputChannels(), inputBufferLength);

    float maxRatio = powf (2.0f, paramShift->getNormalisableRange().start / 12.0f);
    outputBufferLength = (int)floorf ((float)fftSize / maxRatio);
    outputBufferWritePosition = 0;
    outputBufferReadPosition = 0;
    outputBuffer.clear();
    outputBuffer.setSize (getTotalNumInputChannels(), outputBufferLength);

    fftWindow.realloc (fftSize);
    fftWindow.clear (fftSize);

    fftTimeDomain.realloc (fftSize);
    fftTimeDomain.clear (fftSize);

    fftFrequencyDomain.realloc (fftSize);
    fftFrequencyDomain.clear (fftSize);

    samplesSinceLastFFT = 0;

    //======================================

    omega.realloc (fftSize);
    for (int index = 0; index < fftSize; ++index)
        omega[index] = 2.0f * M_PI * index / (float)fftSize;

    inputPhase.clear();
    inputPhase.setSize (getTotalNumInputChannels(), outputBufferLength);

    outputPhase.clear();
    outputPhase.setSize (getTotalNumInputChannels(), outputBufferLength);
}

void PitchShiftAudioProcessor::updateHopSize()
{
    overlap = (int) *paramHopSize;
    if (overlap != 0) {
        hopSize = fftSize / overlap;
        outputBufferWritePosition = hopSize % outputBufferLength;
    }
}

void PitchShiftAudioProcessor::updateAnalysisWindow()
{
    updateWindow (fftWindow, fftSize);
}

void PitchShiftAudioProcessor::updateWindow (const HeapBlock<float>& window, const int windowLength)
{
    switch ((int) *paramWindowType) {
        case windowTypeBartlett: {
            for (int sample = 0; sample < windowLength; ++sample)
                window[sample] = 1.0f - fabs (2.0f * (float)sample / (float)(windowLength - 1) - 1.0f);
            break;
        }
        case windowTypeHann: {
            for (int sample = 0; sample < windowLength; ++sample)
                window[sample] = 0.5f - 0.5f * cosf (2.0f * M_PI * (float)sample / (float)(windowLength - 1));
            break;
        }
        case windowTypeHamming: {
            for (int sample = 0; sample < windowLength; ++sample)
                window[sample] = 0.54f - 0.46f * cosf (2.0f * M_PI * (float)sample / (float)(windowLength - 1));
            break;
        }
    }
}

void PitchShiftAudioProcessor::updateWindowScaleFactor()
{
    float windowSum = 0.0f;
    for (int sample = 0; sample < fftSize; ++sample)
        windowSum += fftWindow[sample];

    windowScaleFactor = 0.0f;
    if (overlap != 0 && windowSum != 0.0f)
        windowScaleFactor = 1.0f / (float)overlap / windowSum * (float)fftSize;
}

float PitchShiftAudioProcessor::princArg (const float phase)
{
    if (phase >= 0.0f)
        return fmod (phase + M_PI,  2.0f * M_PI) - M_PI;
    else
        return fmod (phase + M_PI, -2.0f * M_PI) + M_PI;
}

//==============================================================================


