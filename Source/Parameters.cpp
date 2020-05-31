/*
  ==============================================================================

    Parameters.cpp
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include "Parameters.h"

const Identifier Parameter::IDs::parameterObject = "parameterObject";


Parameter::Parameter(AudioProcessorParameter *param, int type, bool editMode)
    : editMode(editMode)
    , internalPort(true)
    , externalPort(false)
{
    internalPort.setLinkedPort(&externalPort);
    externalPort.setLinkedPort(&internalPort);
    internalPort.incReferenceCount();
    externalPort.incReferenceCount();

    referencedParameter = param;

    outline = Rectangle<int>(10, 20, 130, 90);

    if (param != nullptr) {
        if (param->isBoolean()) {
            // Button
            jassert(type == button || type == null);
            this->type = button;
        } else if (param->isDiscrete() && !param->getAllValueStrings().isEmpty()) {
            // Combo
            jassert(type == combo || type == null);
            this->type = combo;
        } else {
            // Slider
            jassert(type == slider || type == null);
            this->type = slider;
        }
    } else {
        if (type == slider) {
            // Slider
            this->type = slider;
        } else if (type == combo) {
            // Combo
            this->type = combo;
        } else if (type == button) {
            // Button
            this->type = button;
        }
    }

    createParameterComponent();

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

    if (this->type != slider) {
        parameterLabel.setVisible(false);
    }

    parameterLabel.setColour(Label::textColourId, Colours::black);
    parameterLabel.setText(getName(), dontSendNotification);
    //parameterComponent->setTopLeftPosition(0,60);

    externalPort.setCentrePosition(75, 30);

    setEditMode(editMode);
}


void Parameter::positionParameterComponent() {
    if (type == button) {
        parameterComponent->setBounds(20, 30, 100, 30);
    } else if (type == combo) {
        parameterComponent->setBounds(20, 60, 250, 40);
    } else if (type == slider) {
        parameterComponent->setBounds(20, 60, 100, 70);
    }
    parameterLabel.setTopLeftPosition(15, 55);

    setSize(150, 120);
}

void Parameter::createParameterComponent() {
    if (type == button) {
        parameterComponent = std::make_unique<TextButton>();

        auto button = dynamic_cast<TextButton *>(parameterComponent.get());

        if (referencedParameter != nullptr) {
            button->setButtonText(referencedParameter->getName(30));
        }

        button->setToggleState(referencedParameter == nullptr ? false : referencedParameter->getValue(),
                               sendNotificationAsync);
        button->setClickingTogglesState(true);

        if (referencedParameter != nullptr) {
            buttonListener = new ButtonListener(referencedParameter);
            button->addListener(buttonListener);
        }

        button->setName("Button");
        button->setColour(TextButton::ColourIds::textColourOffId, Colours::whitesmoke);
        button->setColour(TextButton::ColourIds::textColourOnId, Colours::red);
        button->setColour(TextButton::buttonColourId, Colours::darkgrey);
        button->setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkslategrey);

    } else if (type == combo) {
        parameterComponent = std::make_unique<ComboBox>();
        auto combo = dynamic_cast<ComboBox *>(parameterComponent.get());

        combo->setName("Combo");

        if (referencedParameter != nullptr) {
            int i = 1;
            for (auto s : referencedParameter->getAllValueStrings()) {
                combo->addItem(s.substring(0, 20), i++);
            }

            comboListener = new ComboListener(referencedParameter);
            combo->addListener(comboListener);

            auto comboParam = dynamic_cast<AudioParameterChoice*>(referencedParameter);
            jassert(comboParam != nullptr);

            combo->setSelectedItemIndex(comboParam->getIndex(), sendNotificationSync);
        } else {
            combo->setSelectedItemIndex(0, sendNotificationSync);
        }
    } else if (type == slider) {
        parameterComponent = std::make_unique<Slider>();
        auto slider = dynamic_cast<Slider *>(parameterComponent.get());

        auto paramRange = NormalisableRange<double>(0, 1);
        if (referencedParameter != nullptr) {
            auto paramRange = dynamic_cast<RangedAudioParameter *>(referencedParameter)->getNormalisableRange();
        }

        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               paramRange.interval, paramRange.skew));

        if (referencedParameter != nullptr) {
            sliderListener = new SliderListener(referencedParameter);
            slider->addListener(sliderListener);
        }

        slider->setName("Slider");
        slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

        slider->setTextBoxIsEditable(true);
        slider->setValue(referencedParameter != nullptr ? referencedParameter->getValue() : 1);

        slider->hideTextBox(false);
        slider->hideTextBox(true);
    }

    addAndMakeVisible(parameterComponent.get());
    positionParameterComponent();
}


void Parameter::parameterValueChanged(int parameterIndex, float newValue) {
    // This is called on audio thread! Use async updater for messages.
    if (editMode && connectedParameter != nullptr) {
        connectedParameter->setValue(newValue, false);
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

    if (editMode) {
        positionParameterComponent();
    } else {
        parameterLabel.setTopLeftPosition(5,5);
        parameterComponent->setTopLeftPosition(5, 5);
        setSize(parameterComponent->getWidth() + 5, 40);
    }

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

    //todo canConnect() checks types for ports
    auto param = otherParameter->getParameter();
    if (param != nullptr) {
        param->addListener(this);

        if (type == slider) {
            if (sliderListener == nullptr) {
                sliderListener = new SliderListener(param);
            }

            auto slider = dynamic_cast<Slider *>(parameterComponent.get());

            slider->addListener(sliderListener);

            if (valueStored) {
                 slider->setValue(value, dontSendNotification);
            } else {
                slider->setValue(param->getValue(), dontSendNotification);
            }
        } else if (type == combo) {
            if (comboListener == nullptr) {
                comboListener = new ComboListener(param);
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
                buttonListener = new ButtonListener(param);
            }

            auto button = dynamic_cast<TextButton *>(parameterComponent.get());

            button->addListener(buttonListener);

            if (valueStored) {
                button->setToggleState(value, dontSendNotification);
            } else {
                button->setToggleState(param->getValue(), dontSendNotification);
            }
        }
    }
}

void Parameter::parameterGestureChanged(int parameterIndex, bool gestureIsStarting) {

}

void Parameter::setValue(float newVal, bool notifyHost) {
    if (referencedParameter != nullptr) {
        if (notifyHost) {
            referencedParameter->setValueNotifyingHost(newVal);
        } else {
            referencedParameter->setValue(newVal);
        }
    } else if (connectedParameter != nullptr) {
        connectedParameter->setValue(newVal, notifyHost);
    } else {
        // Store value for later use
        value = newVal;
        valueStored = true;
    }
}

AudioProcessorParameter *Parameter::getParameter() {
    if (referencedParameter != nullptr) {
        return referencedParameter;
    } else if (connectedParameter != nullptr) {
        return connectedParameter->getParameter();
    } else {
        return nullptr;
    }
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
   //internalPort.setCentrePosition(getX(), getParentComponent()->getHeight() - 15);
    Component::moved();
}

bool Parameter::isConnected() {
    return connectedParameter != nullptr;
}

Parameter* Parameter::getConnectedParameter() {
    return connectedParameter;
}

void Parameter::removeListeners() {
    //referencedParameter->removeListener(this);
}

Parameter::~Parameter() {
    if (sliderListener != nullptr) {
        delete sliderListener;
    }
    if (comboListener != nullptr) {
        delete comboListener;
    }
    if (buttonListener != nullptr) {
        delete buttonListener;
    }
    internalPort.decReferenceCountWithoutDeleting();
    externalPort.decReferenceCountWithoutDeleting();
}

bool Parameter::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
}

bool Parameter::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
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

/*

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


*/
