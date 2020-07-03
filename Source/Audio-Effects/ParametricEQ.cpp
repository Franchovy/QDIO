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

#include "ParametricEQ.h"

//==============================================================================

ParametricEQAudioProcessor::ParametricEQAudioProcessor()
    : BaseEffect("EQ")
    , paramFrequency (new AudioParameterFloat("Frequency", "frequency",
            NormalisableRange<float>(10.0f, 20000.0f, 1.0f), 1500.0f, "Hz"))
                      //[this](float value){ paramFrequency.setCurrentAndTargetValue (value); updateFilters(); return value; })
    , paramQfactor (new AudioParameterFloat("Q Factor", "qfactor",
            NormalisableRange<float>(0.1f, 20.0f, 0.01f), sqrt (2.0f)))
                    //[this](float value){ paramQfactor.setCurrentAndTargetValue (value); updateFilters(); return value; })
    , paramGain (new AudioParameterFloat("Gain", "gain",
            NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 12.0f, "dB"))
                    //[this](float value){ paramGain.setCurrentAndTargetValue (value); updateFilters(); return value; })
    , paramFilterType (new AudioParameterChoice("Filter type", "filtertype", filterTypeItemsUI, filterTypePeakingNotch))
                       //[this](float value){ paramFilterType.setCurrentAndTargetValue (value); updateFilters(); return value; })
{
    setLayout(1,1);

    addParameter(paramFrequency);
    addParameter(paramQfactor);
    addParameter(paramGain);
    addParameter(paramFilterType);

}

ParametricEQAudioProcessor::~ParametricEQAudioProcessor()
{
}

//==============================================================================

void ParametricEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const double smoothTime = 1e-3;
    /*paramFrequency.reset (sampleRate, smoothTime);
    paramQfactor.reset (sampleRate, smoothTime);
    paramGain.reset (sampleRate, smoothTime);
    paramFilterType.reset (sampleRate, smoothTime);*/

    //======================================

    filters.clear();
    for (int i = 0; i < getTotalNumInputChannels(); ++i) {
        Filter* filter;
        filters.add (filter = new Filter());
    }
    updateFilters();
}

void ParametricEQAudioProcessor::releaseResources()
{
}

void ParametricEQAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    //======================================

    for (int channel = 0; channel < numInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer (channel);
        filters[channel]->processSamples (channelData, numSamples);
    }

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

//==============================================================================

void ParametricEQAudioProcessor::updateFilters()
{
    double discreteFrequency = 2.0 * M_PI * (double) *paramFrequency / getSampleRate();
    double qFactor = (double) *paramQfactor;
    double gain = pow (10.0, (double) *paramGain * 0.05);
    int type = (int) *paramFilterType;

    for (int i = 0; i < filters.size(); ++i)
        filters[i]->updateCoefficients (discreteFrequency, qFactor, gain, type);
}

void ParametricEQAudioProcessor::parameterValueChanged(int parameterIndex, float newValue) {
    updateFilters();
}

//==============================================================================
