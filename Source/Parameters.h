/*
  ==============================================================================

    Parameters.h
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Ports.h"

class Parameter : public SelectHoverObject, public AudioProcessorParameter::Listener
{
public:
    Parameter(AudioProcessorParameter *param);

    enum Type {
        button = 0,
        combo = 1,
        slider = 2
    } type;


    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    void setValue(float newVal, bool notifyHost = true);

    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void setEditMode(bool isEditable);
    bool isInEditMode() const;

    //NormalisableRange<double> getRange();

    AudioProcessorParameter* getParameter();

    ParameterPort* getPort(bool internal);
    Point<int> getPortPosition();

    void connect(Parameter* otherParameter);

    struct IDs {
        static const Identifier parameterObject;
    };

    void setActionOnComboSelect(std::function<void()> funct);

private:
    Label parameterLabel;
    std::unique_ptr<Component> parameterComponent;
    AudioProcessorParameter* referencedParameter;

    std::unique_ptr<ParameterPort> internalPort;
    std::unique_ptr<ParameterPort> externalPort;

    bool editMode = false;
    Parameter* connectedParameter = nullptr;

    bool editable = false;
    ComponentDragger dragger;

    Rectangle<int> outline;

};

class ButtonListener : public Button::Listener
{
public:
    explicit ButtonListener(AudioProcessorParameter* parameter) : Button::Listener() {
        linkedParameter = parameter;
    }

    void buttonClicked(Button *button) override {
        linkedParameter->setValue(button->getToggleState());
    }

private:
    AudioProcessorParameter* linkedParameter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonListener)
};


class ComboListener : public ComboBox::Listener
{
public:
    explicit ComboListener(AudioProcessorParameter* parameter) : ComboBox::Listener(){
        linkedParameter = parameter;
    }

    void comboBoxChanged(ComboBox *comboBoxThatHasChanged) override {
        linkedParameter->setValue(comboBoxThatHasChanged->getSelectedItemIndex());
    }

private:
    AudioProcessorParameter* linkedParameter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComboListener)
};

class SliderListener : public Slider::Listener
{
public:
    explicit SliderListener(AudioProcessorParameter* parameter) : Slider::Listener(){
        linkedParameter = parameter;
    }

    void sliderValueChanged(Slider *slider) override {
        linkedParameter->setValue(slider->getValueObject().getValue());
    }

    void sliderDragStarted(Slider *slider) override {
        linkedParameter->beginChangeGesture();
    }

    void sliderDragEnded(Slider *slider) override {
        linkedParameter->endChangeGesture();
    }


private:
    AudioProcessorParameter* linkedParameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderListener)
};

class MetaParameter : public RangedAudioParameter
{
public:
    MetaParameter(String name);

    float getValue() const override;

    void setValue(float newValue) override;

    float getDefaultValue() const override;

    float getValueForText(const String &text) const override;

    bool isAutomatable() const override;

    bool isMetaParameter() const override;

    String getName(int i) const override;

    const NormalisableRange<float> &getNormalisableRange() const override;

    void setLinkedParameter(AudioProcessorParameter* parameter);

private:
    NormalisableRange<float> range;

    AudioProcessorParameter* linkedParameter;
};