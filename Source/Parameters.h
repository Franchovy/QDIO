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

class Parameter;

class ParameterListener
{

};

class ButtonListener : public Button::Listener, public ParameterListener
{
public:
    explicit ButtonListener(Parameter* parameter) : Button::Listener() {
        linkedParameter = parameter;
    }

    void buttonClicked(Button *button) override;

private:
    Parameter* linkedParameter = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonListener)
};


class ComboListener : public ComboBox::Listener, public ParameterListener
{
public:
    explicit ComboListener(Parameter* parameter) : ComboBox::Listener() {
        linkedParameter = parameter;
    }

    void comboBoxChanged(ComboBox *comboBoxThatHasChanged) override;

private:
    Parameter* linkedParameter = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComboListener)
};

class SliderListener : public Slider::Listener, public ParameterListener
{
public:
    explicit SliderListener(Parameter* parameter) : Slider::Listener() {
        linkedParameter = parameter;
    }

    void sliderValueChanged(Slider *slider) override;

    void sliderDragStarted(Slider *slider) override;

    void sliderDragEnded(Slider *slider) override;

    void setReferencedParameter(Parameter* parameter);

private:
    Parameter* linkedParameter = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderListener)
};

class ParameterComponent
{

};

class SliderComponent : public ParameterComponent, public Slider
{

};

class ComboComponent : public ParameterComponent, public ComboBox
{

};

class ButtonComponent : public ParameterComponent, public ToggleButton
{

};

class Parameter : public SelectHoverObject, public AudioProcessorParameter::Listener
{
public:
    using Ptr = ReferenceCountedObjectPtr<Parameter>;

    explicit Parameter(AudioProcessorParameter *param);

    ~Parameter() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    void setName(const String& name) override;

    void setValue(float newVal);

    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void mouseDoubleClick(const MouseEvent &event) override;

    void setEditMode(bool isEditable);
    void setParentEditMode(bool parentIsInEditMode);
    bool isInEditMode() const;

    void moved() override;

    //void parentHierarchyChanged() override;

    void setIsOutput(bool isOutput);
    bool isOutput() const;

    AudioProcessorParameter* getParameter();

    ParameterPort* getPort(bool internal);
    ParameterPort* getPortWithID(String portID);

    void connect(Parameter* otherParameter);
    void disconnect(Parameter* otherParameter);

    void setActionOnComboSelect(std::function<void()> funct);

    void removeListeners();

    bool canDragInto(const SelectHoverObject *other, bool isRightClickDrag = false) const override;
    bool canDragHover(const SelectHoverObject *other, bool isRightClickDrag = false) const override;

protected:
    bool isOutputParameter = false;

    NormalisableRange<double> fullRange;
    NormalisableRange<double> limitedRange;

    Rectangle<int> outline;
    Label parameterLabel;
    bool valueStored = false;
    float value = -1;

    ParameterComponent* parameterComponent;
    ParameterListener* parameterListener;
    AudioProcessorParameter* referencedParameter;

    ParameterPort internalPort;
    ParameterPort externalPort;

    bool parentIsInEditMode = false;
    bool openMode = false;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameter)
};

class SliderParameter : public Parameter
{

private:
    SliderComponent slider;
    SliderListener listener;
};

class ComboParameter : public Parameter
{

private:
    ComboComponent combo;
    ComboListener listener;
};

class ButtonParameter : public Parameter
{
public:

private:
    ButtonComponent button;
    ButtonListener listener;
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
