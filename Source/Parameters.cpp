/*
  ==============================================================================

    Parameters.cpp
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include "Parameters.h"

Parameter::Parameter(AudioProcessorParameter *param)
    : referencedParameter(param)
    , parameterLabel(param->getName(30), param->getName(30))
{
    setBounds(0, 0, 150, 50);

    if (param->isBoolean()) {
        // Button
        type = button;
        parameterComponent = std::make_unique<TextButton>();

    } else if (param->isDiscrete() && !param->getAllValueStrings().isEmpty()) {
        // Combo
        type = combo;
        parameterComponent = std::make_unique<ComboBox>();

    } else {
        // Slider
        type = slider;
        parameterComponent = std::make_unique<Slider>();
        auto slider = dynamic_cast<Slider *>(parameterComponent.get());

        auto paramRange = dynamic_cast<RangedAudioParameter *>(param)->getNormalisableRange();
        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               paramRange.interval, paramRange.skew));

        SliderListener *listener = new SliderListener(param);
        slider->addListener(listener);
        slider->setName("Slider");
        slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);



        slider->setTextBoxIsEditable(true);
        slider->setValue(param->getValue());

        slider->setBounds(0, 0, 100, 70);
        addAndMakeVisible(slider);

        slider->hideTextBox(false);
        slider->hideTextBox(true);

    }

    // Set up label
    parameterLabel.setFont(Font(15, Font::FontStyleFlags::bold));
    parameterLabel.setColour(Label::textColourId, Colours::black);

    parameterLabel.setBounds(0, 0, getWidth(), 20);
    addAndMakeVisible(parameterLabel);

    param->addListener(this);
    param->setValueNotifyingHost(0.0f);
}

void Parameter::parameterValueChanged(int parameterIndex, float newValue) {
    // wot to do with parameterIndex?

    //TODO this is called on audio thread! Use async updater for messages.
    std::cout << "Parameter index: " << parameterIndex << newLine;
    std::cout << "Type: " << type << newLine;

    switch (type) {
        case button:
            break;
        case combo:
            break;
        case slider:
            auto slider = dynamic_cast<Slider*>(parameterComponent.get());
            slider->setValue(newValue, dontSendNotification);
            break;
    }
}
