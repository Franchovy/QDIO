/*
  ==============================================================================

    Parameters.cpp
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include "Parameters.h"

int MetaParameter::nextParameterID = 0;

Parameter::Parameter(AudioProcessorParameter *param)
    : referencedParameter(param)
    , parameterLabel(param->getName(30), param->getName(30))
{
    setBounds(0, 0, 150, 50);

    if (referencedParameter->isMetaParameter()) {
        parameterLabel.setEditable(true, true);
        parameterLabel.setWantsKeyboardFocus(true);
        parameterLabel.showEditor();
    }

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
    parameterLabel.setBounds(0, 0, getWidth(), 20);
    addAndMakeVisible(parameterLabel);

    setEditable(param->isMetaParameter());

    param->addListener(this);
}

void Parameter::parameterValueChanged(int parameterIndex, float newValue) {
    // This is called on audio thread! Use async updater for messages.

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

void Parameter::setEditable(bool isEditable) {
    editable = isEditable;

    if (isEditable) {
        parameterLabel.setEditable(true, true);
        parameterLabel.setColour(Label::textColourId, Colours::whitesmoke);

        parameterComponent->setInterceptsMouseClicks(false, false);
    } else {
        parameterLabel.setEditable(false, false);
        parameterLabel.setColour(Label::textColourId, Colours::black);

        parameterComponent->setInterceptsMouseClicks(true, true);
    }
}

void Parameter::mouseDown(const MouseEvent &event) {
    dragger.startDraggingComponent(this, event);
    Component::mouseDown(event);
}

void Parameter::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);
    Component::mouseDrag(event);
}


MetaParameter::MetaParameter(String name)
        : RangedAudioParameter(name.toUpperCase(), name)
        , range(0, 1, 0.1f)
{
}

float MetaParameter::getValue() const {
    return 0;
}

void MetaParameter::setValue(float newValue) {

}

float MetaParameter::getDefaultValue() const {
    return 0;
}

float MetaParameter::getValueForText(const String &text) const {
    return 0;
}

bool MetaParameter::isAutomatable() const {
    return true;
}

bool MetaParameter::isMetaParameter() const {
    return true;
}

String MetaParameter::getName(int i) const {
    return AudioProcessorParameterWithID::getName(i);
}

const NormalisableRange<float> &MetaParameter::getNormalisableRange() const {
    return range;
}


