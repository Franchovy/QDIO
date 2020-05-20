/*
  ==============================================================================

    Parameters.cpp
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include "Parameters.h"

const Identifier Parameter::IDs::parameterObject = "parameterObject";

int MetaParameter::currentID = 0;

Parameter::Parameter(AudioProcessorParameter *param, bool editMode)
    : editMode(editMode)
    , internalPort(true)
    , externalPort(false)
{
    internalPort.setLinkedPort(&externalPort);
    externalPort.setLinkedPort(&internalPort);

    referencedParameter = param;

    outline = Rectangle<int>(10, 20, 130, 90);

    if (param != nullptr) {
        if (param->isBoolean()) {
            // Button
            type = button;
            parameterComponent = std::make_unique<TextButton>();

        } else if (param->isDiscrete() && !param->getAllValueStrings().isEmpty()) {
            // Combo
            type = combo;
            parameterComponent = std::make_unique<ComboBox>();
            auto combo = dynamic_cast<ComboBox *>(parameterComponent.get());

            int i = 1;
            for (auto s : param->getAllValueStrings()) {
                combo->addItem(s.substring(0, 20), i++);
            }

            auto *listener = new ComboListener(param);
            combo->addListener(listener);
            combo->setName("Combo");

            combo->setSelectedItemIndex(param->getValue());

            combo->setBounds(20, 60, 250, 40);
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

            slider->setBounds(0, 60, 100, 70);
            addAndMakeVisible(slider);

            slider->hideTextBox(false);
            slider->hideTextBox(true);
        }
    } else {
        // Slider
        type = slider;
        parameterComponent = std::make_unique<Slider>();
        auto slider = dynamic_cast<Slider *>(parameterComponent.get());

        auto paramRange = NormalisableRange<double>(0, 1, 0.01);
        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               paramRange.interval, paramRange.skew));

        //auto *listener = new SliderListener(param);
        //slider->addListener(listener);
        slider->setName("Slider");
        slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

        slider->setTextBoxIsEditable(true);

        slider->setBounds(0, 60, 100, 70);
        addAndMakeVisible(slider);

        slider->hideTextBox(false);
        slider->hideTextBox(true);
    }

    if (param != nullptr) {
        param->addListener(this);
    }

    // Set up label
    parameterLabel.setBounds(10, 30, 200, 20);
    parameterLabel.setFont(Font(15, Font::FontStyleFlags::bold));
    parameterLabel.onTextChange = [=] {
        setName(parameterLabel.getText(true));
    };

    if (param != nullptr) {
        setName(param->getName(30));
    } else {
        setName("Parameter");
    }

    addAndMakeVisible(parameterLabel);
    addChildComponent(externalPort);

    if (type == combo) {
        parameterLabel.setVisible(false);
    }

    parameterLabel.setTopLeftPosition(15, 55);
    parameterLabel.setColour(Label::textColourId, Colours::black);
    //parameterComponent->setTopLeftPosition(0,60);

    setBounds(0, 0, 150, 120);

    externalPort.setCentrePosition(75, 30);

    setEditMode(editMode);
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
    externalPort.setVisible(isEditable);
    setHoverable(isEditable);
    parameterLabel.setEditable(isEditable, isEditable);

    parameterLabel.setColour(Label::textColourId,
                             (isEditable ? Colours::whitesmoke : Colours::black));

    editMode = isEditable;

    setInterceptsMouseClicks(editMode, true);

    repaint();
}

void Parameter::mouseDown(const MouseEvent &event) {
    if (event.originalComponent == &externalPort) {
        getParentComponent()->mouseDown(event);
    }
    else if (editMode)
    {
        startDragHoverDetect();
        SelectHoverObject::mouseDown(event);
        dragger.startDraggingComponent(this, event);
    }
}

void Parameter::mouseDrag(const MouseEvent &event) {
    if (event.originalComponent == &externalPort) {
        getParentComponent()->mouseDrag(event);
    }
    else if (editMode)
    {
        dragger.dragComponent(this, event, nullptr);
    }
    SelectHoverObject::mouseDrag(event);
}

void Parameter::mouseUp(const MouseEvent &event) {
    if (event.originalComponent == &externalPort) {
        getParentComponent()->mouseUp(event);
    }
    else if (editMode) {
        endDragHoverDetect();
        SelectHoverObject::mouseUp(event);
    }
}



ParameterPort *Parameter::getPort(bool internal) {
    return internal ? &internalPort : &externalPort;
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

    g.setColour(Colours::blue);
    if (selectMode) {
        g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 3.0f, 3);
    } else if (hoverMode) {
        Path p;
        p.addRoundedRectangle(0, 0, getWidth(), getHeight(), 3.0f);
        PathStrokeType strokeType(3);
        float thiccness[] = {5, 5};
        strokeType.createDashedStroke(p, p, thiccness, 2);
        g.strokePath(p, strokeType);
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
    otherParameter->isConnectedTo = true;
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

void Parameter::moved() {
   internalPort.setCentrePosition(getX(), getParentComponent()->getHeight() - 15);
    Component::moved();
}

bool Parameter::isConnected() {
    return connectedParameter != nullptr;
}

Parameter* Parameter::getConnectedParameter() {
    return connectedParameter;
}

void Parameter::removeListeners() {
    referencedParameter->removeListener(this);
}

Parameter::~Parameter() {

}

bool Parameter::canDragInto(const SelectHoverObject *other) const {
    return false;
}

bool Parameter::canDragHover(const SelectHoverObject *other) const {
    return false;
}

void Parameter::parentHierarchyChanged() {
    if (getParentComponent() != nullptr) {
        getParentComponent()->addAndMakeVisible(internalPort);
    }
    Component::parentHierarchyChanged();
}


/*NormalisableRange<double> Parameter::getRange() {
    auto slider = dynamic_cast<Slider*>(parameterComponent.get());

    return NormalisableRange<double>(slider->getRange(), slider->getInterval());
}*/

MetaParameter::MetaParameter(String name)
        : RangedAudioParameter(newID(name), name)
        , range(0, 1, 0.1f)
{
    linkedParameter = nullptr;
}

float MetaParameter::getValue() const {
    return 0;
}

void MetaParameter::setValue(float newValue) {
    if (linkedParameter != nullptr) {
        linkedParameter->setValueNotifyingHost(newValue);
    }
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

String MetaParameter::newID(String name) {
    return name + String(currentID++);
}


