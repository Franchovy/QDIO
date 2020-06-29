/*
  ==============================================================================

    Parameters.cpp
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include "Parameters.h"


Parameter::Parameter(AudioProcessorParameter *param)
    : internalPort(true, this)
    , externalPort(false, this)
{
    internalPort.setLinkedPort(&externalPort);
    externalPort.setLinkedPort(&internalPort);
    internalPort.incReferenceCount();
    externalPort.incReferenceCount();

    referencedParameter = param;

    outline = Rectangle<int>(10, 20, 130, 90);

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

    parameterLabel.setColour(Label::textColourId, Colours::black);
    parameterLabel.setText(getName(), dontSendNotification);
    //parameterComponent->setTopLeftPosition(0,60);

    externalPort.setCentrePosition(75, 30);

    setEditMode(false);
}



void Parameter::parameterValueChanged(int parameterIndex, float newValue) {
    // This is called on audio thread! Use async updater for messages.
    if (referencedParameter != nullptr && referencedParameter->getValue() != newValue) {
        referencedParameter->setValue(newValue);
    }
    if (connectedParameter != nullptr && connectedParameter->getValue() != newValue) {
        connectedParameter->setValue(newValue);
    }

    if (isOutputParameter) {
        setParameterValueAsync(newValue);
    }
}



void Parameter::setEditMode(bool isEditable) {
    openMode = isEditable;

    // change between editable and non-editable version
    externalPort.setVisible(parentIsInEditMode);
    parameterLabel.setEditable(parentIsInEditMode, parentIsInEditMode);
    parameterLabel.setColour(Label::textColourId,
                             (parentIsInEditMode ? Colours::whitesmoke : Colours::black));

    setHoverable(true);
    setInterceptsMouseClicks(true, true);
    setDraggable(parentIsInEditMode);
}

void Parameter::mouseDown(const MouseEvent &event) {
    if (event.originalComponent == &externalPort) {
        getParentComponent()->mouseDown(event);
    }
}

void Parameter::mouseDrag(const MouseEvent &event) {
    if (event.originalComponent == &externalPort) {
        getParentComponent()->mouseDrag(event);
    }
}

void Parameter::mouseUp(const MouseEvent &event) {
    if (event.originalComponent == &externalPort) {
        getParentComponent()->mouseUp(event);
    }
}



ParameterPort *Parameter::getPort(bool internal) {
    return internal ? &internalPort : &externalPort;
}

void Parameter::paint(Graphics &g) {
    // Draw outline (edit mode)
    if (openMode) {
        Path p;
        g.setColour(Colours::whitesmoke);
        p.addRoundedRectangle(outline, 3.0f);
        PathStrokeType strokeType(2);
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(p, p, thiccness, 2);
        g.strokePath(p, strokeType);
    }

    if (openMode) {
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
    } else {
        g.setColour(Colours::blue);
        if (selectMode) {
            g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 3.0f, 3);
        } else if (hoverMode) {
            g.setColour(Colours::lightgrey);
            g.drawRoundedRectangle(outline.toFloat(), 3, 2);
        }
    }

    /*if (openMode) {
        g.setColour(Colours::lightgrey);
        g.fillRect(getBounds());
    } else {
        g.setColour(Colours::whitesmoke);
        g.fillRect(getBounds());
    }*/

    Component::paint(g);
}

/**
 * Set this parameter to update another with its value.
 * Also takes on slider values of other parameter and intialises with current value.
 * @param otherParameter belongs to a lower-down effect.
 */
void Parameter::connect(Parameter *otherParameter) {
    auto otherParam = otherParameter->getParameter();

    if (otherParam != nullptr) {
        otherParam->addListener(this);
    }
    otherParameter->connectedParameter = referencedParameter;

    if (referencedParameter != nullptr) {
        referencedParameter->addListener(otherParameter);
    }
    connectedParameter = otherParam;

    /*
    bool outputConnection = otherParameter->isOutput();

    connectedParameter = otherParameter;
    otherParameter->isConnectedTo = true;

    //todo canConnect() checks types for ports
    AudioProcessorParameter* param = nullptr;

    param = otherParameter->getParameter();

    if (param != nullptr) {
        param->addListener(this);

        if (type == slider) {
            if (sliderListener == nullptr) {
                sliderListener = new SliderListener(this);
            }

            Slider* slider = nullptr;

            if (outputConnection) {
                slider = dynamic_cast<SliderParameter *>(otherParameter->parameterComponent.get());
            } else {
                slider = dynamic_cast<SliderParameter *>(parameterComponent.get());

                // Adopted range of connected param
                fullRange = connectedParameter->limitedRange;
            }

            slider->addListener(sliderListener);
            
            if (! outputConnection) { //todo check this
                if (valueStored) {
                    slider->setValue(value, dontSendNotification);
                } else {
                    slider->setValue(param->getValue(), dontSendNotification);
                }
            }
        } else if (type == combo) {
            if (comboListener == nullptr) {
                comboListener = new ComboListener(this);
            }

            int i = 1;
            auto combo = dynamic_cast<ComboBox *>(parameterComponent.get());
            combo->clear(dontSendNotification);

            for (auto s : param->getAllValueStrings()) {
                combo->addItem(s.substring(0, 20), i++);
            }

            combo->addListener(comboListener);

            if (valueStored) {
                combo->setSelectedId(value, dontSendNotification);
            } else {
                combo->setSelectedId(param->getValue(), dontSendNotification);
            }
        } else if (type == button) {
            if (buttonListener == nullptr) {
                buttonListener = new ButtonListener(this);
            }

            auto button = dynamic_cast<TextButton *>(parameterComponent.get());

            button->addListener(buttonListener);

            if (valueStored) {
                button->setToggleState(value, dontSendNotification);
            } else {
                button->setToggleState(param->getValue(), dontSendNotification);
            }
        }
    }*/

}

void Parameter::disconnect(Parameter* otherParameter) {
    auto otherParam = otherParameter->getParameter();

    if (otherParam != nullptr) {
        otherParam->removeListener(this);
    } else if (otherParameter->connectedParameter == referencedParameter) {
        otherParameter->connectedParameter = nullptr;
    }

    if (referencedParameter != nullptr) {
        referencedParameter->removeListener(otherParameter);
    } else if (connectedParameter == otherParam) {
        connectedParameter = nullptr;
    }



    /*if (toThis) {
        // Disconnect this as target
        isConnectedTo = false;

    } else {
        // Disconnect to whatever this is connected to
        auto param = connectedParameter->getParameter();
        if (param != nullptr) {
            param->removeListener(this);
        }

        if (type == slider) {
            auto slider = dynamic_cast<SliderParameter*>(parameterComponent.get());
            slider->removeListener(sliderListener);
        } else if (type == combo) {
            auto combo = dynamic_cast<ComboBox*>(parameterComponent.get());
            combo->removeListener(comboListener);
            combo->clear(sendNotificationAsync);
        } else if (type == button) {
            auto button = dynamic_cast<ToggleButton*>(parameterComponent.get());
            button->removeListener(buttonListener);
            button->setToggleState(false, sendNotificationAsync);
        }

        if (connectedParameter != nullptr && connectedParameter->isConnectedTo) {
            connectedParameter->disconnect(true);
        }

        connectedParameter = nullptr;
    }*/
}

void Parameter::parameterGestureChanged(int parameterIndex, bool gestureIsStarting) {

}

AudioProcessorParameter *Parameter::getParameter() {
    if (referencedParameter != nullptr) {
        return referencedParameter;
    } else if (connectedParameter != nullptr) {
        return connectedParameter;
    } else {
        return nullptr;
    }
}

/*void Parameter::setActionOnComboSelect(std::function<void()> funct) {
    if (type == combo) {
        auto combo = dynamic_cast<ComboBox*>(parameterComponent.get());
        combo->onChange = std::move(funct);
    }
}*/

bool Parameter::isInEditMode() const {
    return openMode;
}

void Parameter::moved() {
    if (openMode) {
        internalPort.setCentrePosition(getX() + getWidth() / 2, getParentComponent()->getHeight() - 20);
    }
    Component::moved();
}

Parameter::~Parameter() {
    internalPort.decReferenceCountWithoutDeleting();
    externalPort.decReferenceCountWithoutDeleting();
}

bool Parameter::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
}

bool Parameter::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false; //todo return other is Parameter && output-input match
}

void Parameter::parentHierarchyChanged() {
    if (getParentComponent() != nullptr) {
        getParentComponent()->addAndMakeVisible(internalPort);
    }
    Component::parentHierarchyChanged();
}

ParameterPort *Parameter::getPortWithID(String portID) {
    if (internalPort.getComponentID() == portID) {
        return &internalPort;
    } else if (externalPort.getComponentID() == portID) {
        return &externalPort;
    }
    return nullptr;
}

void Parameter::setName(const String& name) {
    parameterLabel.setText(name, dontSendNotification);
    Component::setName(name);
}

void Parameter::mouseDoubleClick(const MouseEvent &event) {
    setEditMode(! openMode);
    removeSelectObject(this);

    Component::mouseDoubleClick(event);
}

void Parameter::setIsOutput(bool isOutput) {
    isOutputParameter = isOutput;
    internalPort.isInput = ! isOutput;
}

bool Parameter::isOutput() const {
    return isOutputParameter;
}

void Parameter::setParentEditMode(bool parentIsInEditMode) {
    this->parentIsInEditMode = parentIsInEditMode;
    setEditMode(parentIsInEditMode);
}

void ButtonListener::buttonClicked(Button *button) {
    if (linkedParameter != nullptr) {
        linkedParameter->setValueNotifyingHost(button->getToggleState());
    }
}

void ComboListener::comboBoxChanged(ComboBox *comboBoxThatHasChanged) {
    if (linkedParameter != nullptr
            && linkedParameter->getCurrentValueAsText().compare(comboBoxThatHasChanged->getText()) != 0) {
        linkedParameter->setValueNotifyingHost(comboBoxThatHasChanged->getSelectedId());
    }
}

void SliderListener::sliderValueChanged(Slider *slider) {
    /*if (dynamic_cast<Parameter*>(slider->getParentComponent())->isOutput()) {
        auto normalisedRange = NormalisableRange<double>(slider->getRange().getStart(), slider->getRange().getEnd());
        linkedParameter->setValueNormalised(normalisedRange.convertTo0to1(slider->getValue()));
    } else {
        linkedParameter->setValueDirect(slider->getValue());
    }*/
    if (linkedParameter != nullptr) {
        linkedParameter->setValueNotifyingHost(slider->getValue());
    }
}

void SliderListener::sliderDragStarted(Slider *slider) {
    /*auto param = linkedParameter->getParameter();
    if (param != nullptr) {
        param->beginChangeGesture();
    }*/
    Listener::sliderDragStarted(slider);
}

void SliderListener::sliderDragEnded(Slider *slider) {
    /*auto param = linkedParameter->getParameter();
    if (param != nullptr) {
        param->endChangeGesture();
    }*/
    Listener::sliderDragEnded(slider);
}

/*bool SliderParameter::hitTest(int x, int y) {
    return (Rectangle<int>(10, 30, getWidth()-10, getHeight() - 30).contains(x, y));
}*/

SliderParameter::SliderParameter(AudioProcessorParameter* param) : Parameter(param)
        , listener(referencedParameter)
{
    fullRange = NormalisableRange<double>(0, 1);
    if (referencedParameter != nullptr) {
        auto range = dynamic_cast<RangedAudioParameter *>(referencedParameter)->getNormalisableRange();
        fullRange = NormalisableRange<double>(range.start, range.end, range.interval, range.skew);
        slider.setTextValueSuffix(referencedParameter->getLabel());
    }
    limitedRange = NormalisableRange<double>(fullRange);

    slider.setNormalisableRange(fullRange);

    if (referencedParameter != nullptr) {
        slider.addListener(&listener);
    }

    slider.setName("Slider");

    slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    slider.setTextBoxIsEditable(true);

    slider.setValue(referencedParameter != nullptr ? referencedParameter->getValue() : 1);
    //slider->setSliderStyle(Slider::SliderStyle::ThreeValueHorizontal);

    slider.setPopupDisplayEnabled(true, true,
                                   getParentComponent(), -1);


    slider.hideTextBox(false);
    slider.hideTextBox(true);

    setBounds(0, 0, 130, 40);
    slider.setBounds(20, 60, 100, 70);

    parameterLabel.setTopLeftPosition(5,5);
    slider.setTopLeftPosition(5, 0);
    setSize(getWidth(), 40);

    addAndMakeVisible(slider);
}


void SliderParameter::setEditMode(bool isEditable) {
    auto value = slider.getValue();

    if (openMode) {
        slider.setSliderStyle(Slider::SliderStyle::ThreeValueHorizontal);
        slider.setNormalisableRange(fullRange);
        slider.setMinAndMaxValues(limitedRange.start, limitedRange.end);
        slider.setValue(limitedRange.snapToLegalValue(value));
    } else {
        if (slider.getSliderStyle() == Slider::ThreeValueHorizontal) {
            limitedRange.start = slider.getMinValue();
            limitedRange.end = slider.getMaxValue();
        }

        slider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        slider.setNormalisableRange(limitedRange);
        slider.setValue(limitedRange.snapToLegalValue(value));
    }

    Parameter::setEditMode(isEditable);
}

void SliderParameter::connect(Parameter *otherParameter) {
    Parameter::connect(otherParameter);
}

void SliderParameter::disconnect(Parameter *otherParameter) {
    Parameter::disconnect(otherParameter);
}

void SliderParameter::setParameterValueAsync(float value) {
    auto sliderRange = slider.getRange();
    slider.setValue(value, sendNotificationAsync);
}

ComboParameter::ComboParameter(AudioProcessorParameter* param) : Parameter(param)
        , listener(referencedParameter)
{
    combo.setName("Combo");

    if (referencedParameter != nullptr) {
        int i = 1;
        for (auto s : referencedParameter->getAllValueStrings()) {
            combo.addItem(s.substring(0, 20), i++);
        }

        combo.addListener(&listener);

        auto comboParam = dynamic_cast<AudioParameterChoice*>(referencedParameter);
        jassert(comboParam != nullptr);

        combo.setSelectedItemIndex(*comboParam, sendNotificationSync);
    } else {
        combo.setSelectedItemIndex(0, sendNotificationSync);
    }

    setBounds(0,0,200, 55);
    combo.setBounds(5, 10, 190, 40);

    outline = Rectangle<int>();

    parameterLabel.setTopLeftPosition(5,5);
    combo.setTopLeftPosition(5, 0);
    setSize(getWidth(), 40);

    addAndMakeVisible(combo);
}

void ComboParameter::setEditMode(bool isEditable) {
    Parameter::setEditMode(isEditable);
}

void ComboParameter::connect(Parameter *otherParameter) {
    Parameter::connect(otherParameter);
}

void ComboParameter::disconnect(Parameter *otherParameter) {
    Parameter::disconnect(otherParameter);
}

void ComboParameter::setParameterValueAsync(float value) {
    combo.setSelectedId((int) value, sendNotificationAsync);
}

ButtonParameter::ButtonParameter(AudioProcessorParameter* param) : Parameter(param)
    , listener(referencedParameter)
{
    if (referencedParameter != nullptr) {
        button.setButtonText(referencedParameter->getName(30));
    }

    button.setToggleState(referencedParameter == nullptr ? false : referencedParameter->getValue(),
                           sendNotificationSync);
    button.setClickingTogglesState(true);

    if (referencedParameter != nullptr) {
        button.addListener(&listener);
    }

    button.setName("Button");
    button.setColour(TextButton::ColourIds::textColourOffId, Colours::whitesmoke);
    button.setColour(TextButton::ColourIds::textColourOnId, Colours::red);
    button.setColour(TextButton::buttonColourId, Colours::darkgrey);
    button.setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkslategrey);

    outline = Rectangle<int>();

    setBounds(0,0,120, 45);
    button.setBounds(20, 10, 100, 30);

    setSize(150, 120);

    parameterLabel.setTopLeftPosition(5,5);
    button.setTopLeftPosition(5, 0);
    setSize(getWidth(), 40);

    addAndMakeVisible(button);
}

void ButtonParameter::setEditMode(bool isEditable) {
    Parameter::setEditMode(isEditable);
}

void ButtonParameter::connect(Parameter *otherParameter) {
    Parameter::connect(otherParameter);
}

void ButtonParameter::disconnect(Parameter *otherParameter) {
    Parameter::disconnect(otherParameter);
}

void ButtonParameter::setParameterValueAsync(float value) {
    button.setToggleState((bool) value, sendNotificationAsync);
}
