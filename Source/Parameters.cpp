/*
  ==============================================================================

    Parameters.cpp
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include "Parameters.h"

const Identifier Parameter::IDs::parameterComponent = "parameterObject";

Parameter::Parameter(AudioProcessorParameter *param)
    : referencedParameter(param)
    , internal(! param->isMetaParameter())
    , parameterLabel(param->getName(30), param->getName(30))
    , port(std::make_unique<ParameterPort>(param, internal))
{
    setBounds(0, 0, 150, 80);

    if (referencedParameter->isMetaParameter()) {
        parameterLabel.setEditable(true, true);
        //parameterLabel.setWantsKeyboardFocus(true);
        //parameterLabel.showEditor();
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

    setEditable(! internal);

    if (! internal) {
        port->setCentrePosition(getWidth()/2, 0);
    }
    port->setParentParameter(this);

    param->addListener(this);
}

void Parameter::parameterValueChanged(int parameterIndex, float newValue) {
    // This is called on audio thread! Use async updater for messages.
    if (! internal && connectedParameter != nullptr) {
        connectedParameter->setValue(newValue);
    }


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
        port->setVisible(true);

        parameterLabel.setEditable(true, true);
        parameterLabel.setColour(Label::textColourId, Colours::whitesmoke);

        parameterComponent->setInterceptsMouseClicks(false, false);
    } else {
        if (! internal) {
            port->setVisible(false);
        }

        parameterLabel.setEditable(false, false);
        parameterLabel.setColour(Label::textColourId, Colours::black);

        parameterComponent->setInterceptsMouseClicks(true, true);
    }
    repaint();
}

void Parameter::mouseDown(const MouseEvent &event) {
    dragger.startDraggingComponent(this, event);
    Component::mouseDown(event);
}

void Parameter::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);
    Component::mouseDrag(event);
}

ParameterPort *Parameter::getPort() {
    return port.get();
}

Point<int> Parameter::getPortPosition() {
    // Default: return 20 above centre-top
    return Point<int>(getWidth()/2, 0);
}

bool Parameter::isInternal() {
    return internal;
}

void Parameter::paint(Graphics &g) {
    // Draw outline (edit mode)
    if (editable) {
        Path p;
        g.setColour(Colours::whitesmoke);
        p.addRoundedRectangle(0, 0, getWidth(), getHeight(), 3.0f);
        PathStrokeType strokeType(2);
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(p, p, thiccness, 2);
        g.strokePath(p, strokeType);
    }

    Component::paint(g);
}

void Parameter::connect(Parameter *otherParameter) {
    Parameter* internalParam;
    Parameter* externalParam;
    if (internal) {
        internalParam = this;
        externalParam = otherParameter;
    } else {
        externalParam = this;
        internalParam = otherParameter;
    }

    connectedParameter = otherParameter;
}

void Parameter::parameterGestureChanged(int parameterIndex, bool gestureIsStarting) {

}

void Parameter::setValue(float newVal, bool notifyHost) {
    if (notifyHost) {
        referencedParameter->setValueNotifyingHost(newVal);
    } else {
        referencedParameter->setValue(newVal);
    }
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


