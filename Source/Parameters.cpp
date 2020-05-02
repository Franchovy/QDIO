/*
  ==============================================================================

    Parameters.cpp
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include "Parameters.h"

const Identifier Parameter::IDs::parameterObject = "parameterObject";

Parameter::Parameter(AudioProcessorParameter *param)
    : referencedParameter(param)
    , editMode(param->isMetaParameter())
    , parameterLabel(param->getName(30), param->getName(30))
    , internalPort(std::make_unique<ParameterPort>(true))
    , externalPort(std::make_unique<ParameterPort>(false))
{

    //TODO use parameterListener
    //param->addListener(this);

    if (! editMode) {
        setBounds(0, 0, 150, 80);
    } else {
        setBounds(0, 0, 150, 120);
        outline = Rectangle<int>(10, 20, 130, 90);
    }

    // IF EDIT MODE
    // parameterLabel.setEditMode(true, true);

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
    parameterLabel.onTextChange = [=] {
        setName(parameterLabel.getText(true));
    };

    addAndMakeVisible(parameterLabel);
    addChildComponent(externalPort.get());

    if (type == slider) {
        internalPort->setCentrePosition(getWidth() / 2, 30);
        parameterLabel.setTopLeftPosition(15, 55);
        parameterComponent->setCentrePosition(getWidth()/2, 80);
    }

    if (type == combo) {
        parameterLabel.setVisible(false);
    }

    param->addListener(this);
}

void Parameter::parameterValueChanged(int parameterIndex, float newValue) {
    // This is called on audio thread! Use async updater for messages.
    if (editMode && connectedParameter != nullptr) {
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

void Parameter::setEditMode(bool isEditable) {
    // change between editable and non-editable version
    internalPort->setVisible(isEditable);
    setHoverable(isEditable);
    parameterLabel.setEditable(isEditable, isEditable);

    parameterLabel.setColour(Label::textColourId,
                             (isEditable ? Colours::whitesmoke : Colours::black));

    editMode = isEditable;

    repaint();
}

void Parameter::mouseDown(const MouseEvent &event) {
    if (event.originalComponent == internalPort.get()) {
        getParentComponent()->mouseDown(event);
    }
    else if (editMode)
    {
        dragger.startDraggingComponent(this, event);
    }
    SelectHoverObject::mouseDown(event);
}

void Parameter::mouseDrag(const MouseEvent &event) {
    if (event.originalComponent == internalPort.get()) {
        getParentComponent()->mouseDrag(event);
    }
    else if (editMode)
    {
        dragger.dragComponent(this, event, nullptr);
    }
    SelectHoverObject::mouseDrag(event);
}

void Parameter::mouseUp(const MouseEvent &event) {
    if (event.originalComponent == internalPort.get()) {
        getParentComponent()->mouseUp(event);
    }
    SelectHoverObject::mouseUp(event);
}



ParameterPort *Parameter::getPort(bool internal) {
    return internal ? internalPort.get() : externalPort.get();
}

Point<int> Parameter::getPortPosition() {
    // Default: return 20 above centre-top
    return Point<int>(getWidth()/2, 0);
}

void Parameter::paint(Graphics &g) {
    // Draw outline (edit mode)
    if (editMode) {
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
 * Set this parameter to update another with its value.
 * Also takes on slider values of other parameter and intialises with current value.
 * @param otherParameter belongs to a lower-down effect.
 */
void Parameter::connect(Parameter *otherParameter) {
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

bool Parameter::isInEditMode() const {
    return editMode;
}

/*NormalisableRange<double> Parameter::getRange() {
    auto slider = dynamic_cast<Slider*>(parameterComponent.get());

    return NormalisableRange<double>(slider->getRange(), slider->getInterval());
}*/

MetaParameter::MetaParameter(String name)
        : RangedAudioParameter(name.toLowerCase(), name)
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


