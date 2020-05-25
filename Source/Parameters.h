/*
  ==============================================================================

    Parameters.h
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Ports.h"


#pragma once

class ButtonListener : public Button::Listener
{
public:
    explicit ButtonListener(AudioProcessorParameter* parameter) : Button::Listener() {
        linkedParameter = parameter;
    }

    void buttonClicked(Button *button) override {
        linkedParameter->setValueNotifyingHost(button->getToggleState());
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
        linkedParameter->setValueNotifyingHost(comboBoxThatHasChanged->getSelectedItemIndex());
    }

private:
    AudioProcessorParameter* linkedParameter = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComboListener)
};

class SliderListener : public Slider::Listener
{
public:
    explicit SliderListener(AudioProcessorParameter* parameter) : Slider::Listener(){
        linkedParameter = parameter;
    }

    void sliderValueChanged(Slider *slider) override {
        linkedParameter->setValueNotifyingHost(slider->getValueObject().getValue());
    }

    void sliderDragStarted(Slider *slider) override {

        linkedParameter->beginChangeGesture();
    }

    void sliderDragEnded(Slider *slider) override {
        linkedParameter->endChangeGesture();
    }


private:
    AudioProcessorParameter* linkedParameter = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderListener)
};



class Parameter : public SelectHoverObject, public AudioProcessorParameter::Listener
{
public:
    using Ptr = ReferenceCountedObjectPtr<Parameter>;

    explicit Parameter(AudioProcessorParameter *param = nullptr, int type = slider, bool editMode = false);

    ~Parameter() override;

    enum Type {
        button = 0,
        combo = 1,
        slider = 2
    } type;


    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    void setName(const String& name) override;

    void setValue(float newVal, bool notifyHost = true);

    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void setEditMode(bool isEditable);
    bool isInEditMode() const;

    void moved() override;

    void parentHierarchyChanged() override;

    bool isConnected();
    Parameter* getConnectedParameter();

    AudioProcessorParameter* getParameter();

    ParameterPort* getPort(bool internal);
    ParameterPort* getPortWithID(String portID);

    Point<int> getPortPosition();

    void connect(Parameter* otherParameter);

    struct IDs {
        static const Identifier parameterObject;
    };

    void setActionOnComboSelect(std::function<void()> funct);

    void removeListeners();

    bool canDragInto(const SelectHoverObject *other) const override;
    bool canDragHover(const SelectHoverObject *other) const override;

private:
    Label parameterLabel;
    std::unique_ptr<Component> parameterComponent;
    AudioProcessorParameter* referencedParameter;
    bool valueStored = false;
    float value = -1;

    SliderListener* sliderListener = nullptr;
    ComboListener* comboListener = nullptr;
    ButtonListener* buttonListener = nullptr;

    ParameterPort internalPort;
    ParameterPort externalPort;

    bool editMode = false;
    Parameter* connectedParameter = nullptr;
    bool isConnectedTo = false;

    bool editable = false;
    ComponentDragger dragger;

    Rectangle<int> outline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameter)
};


/*
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

    String newID(String name);

private:
    NormalisableRange<float> range;

    AudioProcessorParameter* linkedParameter = nullptr;

    static int currentID;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetaParameter)
};*/
