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

#include "RobotizationWhisperization.h"

//==============================================================================

RobotizationWhisperizationAudioProcessor::RobotizationWhisperizationAudioProcessor():
    BaseEffect()
    , stft(*this)
    , paramEffect (new AudioParameterChoice("Effect", "effect", effectItemsUI, effectPassThrough))
    , paramFftSize (new AudioParameterChoice("FFT size", "fftsize", fftSizeItemsUI, fftSize512))
                    /*[this](float value){
                        const ScopedLock sl (lock);
                        value = (float)(1 << ((int)value + 5));
                        paramFftSize.setCurrentAndTargetValue (value);
                        stft.updateParameters((int)paramFftSize.getTargetValue(),
                                              (int)paramHopSize.getTargetValue(),
                                              (int)paramWindowType.getTargetValue());
                        return value;
                    })*/
    , paramHopSize (new AudioParameterChoice("Hop size", "hopsize", hopSizeItemsUI, hopSize8))
                    /*[this](float value){
                        const ScopedLock sl (lock);
                        value = (float)(1 << ((int)value + 1));
                        paramHopSize.setCurrentAndTargetValue (value);
                        stft.updateParameters((int)paramFftSize.getTargetValue(),
                                              (int)paramHopSize.getTargetValue(),
                                              (int)paramWindowType.getTargetValue());
                        return value;
                    })*/
    , paramWindowType (new AudioParameterChoice("Window type", "windowtype", windowTypeItemsUI, STFT::windowTypeHann))
                       /*[this](float value){
                           const ScopedLock sl (lock);
                           paramWindowType.setCurrentAndTargetValue (value);
                           stft.updateParameters((int)paramFftSize.getTargetValue(),
                                                 (int)paramHopSize.getTargetValue(),
                                                 (int)paramWindowType.getTargetValue());
                           return value;
                       })*/
{
    setLayout(1,1);

    addParameter(paramEffect);
    *paramFftSize = fftSize64;
    //addParameter(paramFftSize);
    addParameter(paramHopSize);
    addParameter(paramWindowType);

    addRefreshParameterFunction([=] {
        stft.updateParameters((int)(float)(1 << ((int)*paramFftSize + 5)),
                              (int)(float)(1 << ((int)*paramHopSize + 1)),
                              (int)*paramWindowType);
    });
    setRefreshRate(2);
}

RobotizationWhisperizationAudioProcessor::~RobotizationWhisperizationAudioProcessor()
{
}

//==============================================================================

void RobotizationWhisperizationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramEffect.reset (sampleRate, smoothTime);
    paramFftSize.reset (sampleRate, smoothTime);
    paramHopSize.reset (sampleRate, smoothTime);
    paramWindowType.reset (sampleRate, smoothTime);*/

    //======================================

    stft.setup (getTotalNumInputChannels());
    stft.updateParameters((int)(float)(1 << ((int)*paramFftSize + 5)),
                          (int)(float)(1 << ((int)*paramHopSize + 1)),
                          (int)*paramWindowType);
}

void RobotizationWhisperizationAudioProcessor::releaseResources()
{
}

void RobotizationWhisperizationAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const ScopedLock sl (lock);

    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    stft.processBlock (buffer);

    //======================================

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}
