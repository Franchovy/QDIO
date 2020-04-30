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

    //TODO use parameterListener
    //param->addListener(this);

    if (internal) {
        setBounds(0, 0, 150, 80);
    } else {
        setBounds(0, 0, 150, 120);
        outline = Rectangle<int>(10, 20, 130, 90);
    }

    if (referencedParameter->isMetaParameter()) {
        parameterLabel.setEditable(true, true);
        //parameterLabel.setWantsKeyboardFocus(true);
        //parameterLabel.showEditor();
    }

    if (param->isBoolean()) {
        // Button
        type = button;
        parameterComponent = std::make_unique<TextButton>();

    } else if (param->isDiscrete() && ! param->getAllValueStrings().isEmpty()) {
        // Combo
        type = combo;
        parameterComponent = std::make_unique<ComboBox>();
        auto combo = dynamic_cast<ComboBox*>(parameterComponent.get());

        int i = 1;
        for (auto s : param->getAllValueStrings()) {
            combo->addItem(s.substring(0, 20), i++);
        }

        auto *listener = new ComboListener(param);
        combo->addListener(listener);
        combo->setName("Combo");

        combo->setSelectedItemIndex(param->getValue());

        combo->setBounds(20, 0, 250, 40);
        addAndMakeVisible(combo);
    } else {
        // Slider
        type = slider;
        parameterComponent = std::make_unique<Slider>();
        auto slider = dynamic_cast<Slider *>(parameterComponent.get());

        auto paramRange = dynamic_cast<RangedAudioParameter *>(param)->getNormalisableRange();
        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               paramRange.interval, paramRange.skew));

        auto *listener = new SliderListener(param);
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
    parameterLabel.setBounds(0, 0, getWidth(), 20);
    parameterLabel.setFont(Font(15, Font::FontStyleFlags::bold));

    addAndMakeVisible(parameterLabel);

    setEditable(! internal);

    if (! internal) {
        if (type == slider) {
            port->setCentrePosition(getWidth()/2, 30);
            parameterLabel.setTopLeftPosition(15, 55);
            parameterComponent->setCentrePosition(getWidth()/2, 80);
        }
    }
    if (type == combo) {
        parameterLabel.setVisible(false);
    }

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
        port->setInterceptsMouseClicks(true, true);

        setHoverable(true);

        parameterLabel.setEditable(true, true);
        parameterLabel.setColour(Label::textColourId, Colours::whitesmoke);
    } else {
        if (! internal) {
            port->setVisible(false);
        }

        setHoverable(false);

        parameterLabel.setEditable(false, false);
        parameterLabel.setColour(Label::textColourId, Colours::black);
    }
    repaint();
}

void Parameter::mouseDown(const MouseEvent &event) {
    if (event.originalComponent == port.get()) {
        getParentComponent()->mouseDown(event);
    }
    else if (! internal)
    {
        dragger.startDraggingComponent(this, event);
    }
    SelectHoverObject::mouseDown(event);
}

void Parameter::mouseDrag(const MouseEvent &event) {
    if (event.originalComponent == port.get()) {
        getParentComponent()->mouseDrag(event);
    }
    else if (! internal)
    {
        dragger.dragComponent(this, event, nullptr);
    }
    SelectHoverObject::mouseDrag(event);
}

void Parameter::mouseUp(const MouseEvent &event) {
    if (event.originalComponent == port.get()) {
        getParentComponent()->mouseUp(event);
    }
    SelectHoverObject::mouseUp(event);
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
        p.addRoundedRectangle(outline, 3.0f);
        PathStrokeType strokeType(2);
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(p, p, thiccness, 2);
        g.strokePath(p, strokeType);
    }

    if (hoverMode) {
        Path p;
        g.setColour(Colours::blue);
        p.addRoundedRectangle(0, 0, getWidth(), getHeight(), 3.0f);
        PathStrokeType strokeType(2);
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(p, p, thiccness, 2);
        g.strokePath(p, strokeType);
    }

    if (selectMode) {
        g.setColour(Colours::blue);
        g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 3.0f, 2);
    }

    Component::paint(g);
}

/**
 * Set this parameter to update another with its value. Also takes on slider
 * values of other parameter.
 * @param otherParameter
 */
void Parameter::connect(Parameter *otherParameter) {
    jassert(! internal);

    connectedParameter = otherParameter;
    dynamic_cast<MetaParameter*>(referencedParameter)->setLinkedParameter(connectedParameter->getParameter());

    referencedParameter->setValueNotifyingHost(connectedParameter->getParameter()->getValue());

    /*auto slider = dynamic_cast<Slider*>(parameterComponent.get());
    slider->setNormalisableRange(otherParameter->getRange());*/
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

AudioProcessorParameter *Parameter::getParameter() {
    return referencedParameter;
}

void Parameter::setActionOnComboSelect(std::function<void()> funct) {
    if (type == combo) {
        auto combo = dynamic_cast<ComboBox*>(parameterComponent.get());
        combo->onChange = funct;
    }
}

/*NormalisableRange<double> Parameter::getRange() {
    auto slider = dynamic_cast<Slider*>(parameterComponent.get());

    return NormalisableRange<double>(slider->getRange(), slider->getInterval());
}*/

MetaParameter::MetaParameter(String name)
        : RangedAudioParameter(name.toUpperCase(), name)
        , range(0, 1, 0.1f)
{
}

float MetaParameter::getValue() const {
    return 0;
}

void MetaParameter::setValue(float newValue) {
    linkedParameter->setValueNotifyingHost(newValue);
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

void MetaParameter::setLinkedParameter(AudioProcessorParameter *parameter) {
    linkedParameter = parameter;
}


