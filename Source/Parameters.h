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
    explicit ButtonListener(AudioProcessorParameter* parameter) : Button::Listener() {
        linkedParameter = dynamic_cast<AudioParameterBool*>(parameter);
    }

    void buttonClicked(Button *button) override;

private:
    AudioParameterBool* linkedParameter = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonListener)
};


class ComboListener : public ComboBox::Listener, public ParameterListener
{
public:
    explicit ComboListener(AudioProcessorParameter* parameter) : ComboBox::Listener() {
        linkedParameter = dynamic_cast<AudioParameterChoice*>(parameter);
    }

    void comboBoxChanged(ComboBox *comboBoxThatHasChanged) override;

private:
    AudioParameterChoice* linkedParameter = nullptr;
    bool valueFloat = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComboListener)
};

class SliderListener : public Slider::Listener, public ParameterListener
{
public:
    explicit SliderListener(Parameter* parameter);

    void sliderValueChanged(Slider *slider) override;

    void sliderDragStarted(Slider *slider) override;

    void sliderDragEnded(Slider *slider) override;

private:
    bool manualControl = false;
    Parameter* parent;
    AudioParameterFloat* linkedParameter = nullptr;

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

class ButtonComponent : public ParameterComponent, public TextButton
{

};

class ParameterUpdater : public Timer
{
public:
    ParameterUpdater();

    void timerCallback() override;

    void addParameter(Parameter* parameter);

private:
    Array<Parameter*> parametersToUpdate;
};

class Parameter : public SelectHoverObject, public AudioProcessorParameter::Listener, public AsyncUpdater
{
public:
    using Ptr = ReferenceCountedObjectPtr<Parameter>;

    explicit Parameter(AudioProcessorParameter *param);

    ~Parameter() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    void addToUpdater();

    void handleAsyncUpdate() override;

    void setName(const String& name) override;

    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void mouseDoubleClick(const MouseEvent &event) override;

    virtual void setEditMode(bool isEditable);
    virtual void setParentEditMode(bool parentIsInEditMode);
    bool isInEditMode() const;

    void moved() override;

    virtual void setParameterValueAsync(float value) = 0;
    virtual void setValueSync(float value) = 0;
    void parentHierarchyChanged() override;

    bool isMetaParameter();

    void setIsOutput(bool isOutput);
    bool isOutput() const;

    AudioProcessorParameter* getParameter();

    ParameterPort* getPort(bool internal);
    ParameterPort* getPortWithID(String portID);

    virtual void connect(Parameter* otherParameter);
    virtual void disconnect(Parameter* otherParameter);

    bool isConnected();
    Parameter* getConnectedParameter();

    //void setActionOnComboSelect(std::function<void()> funct);

    bool canDragInto(const SelectHoverObject *other, bool isRightClickDrag = false) const override;
    bool canDragHover(const SelectHoverObject *other, bool isRightClickDrag = false) const override;

    NormalisableRange<double> getFullRange();
    NormalisableRange<double> getLimitedRange();

    static ParameterUpdater* updater;

protected:
    bool initialised = false;

    bool isOutputParameter = false;
    bool isAuto = false;

    bool metaParameter = false;

    NormalisableRange<double> fullRange;
    NormalisableRange<double> limitedRange;

    Rectangle<int> outline;
    Label parameterLabel;
    bool valueStored = false;
    float value = -1;

    ParameterComponent* parameterComponent;
    ParameterListener* parameterListener;
    
    AudioProcessorParameter* referencedParam = nullptr;
    // Parameter to update on slider value changes -- in the meantime of every Parameter containing its own
    AudioProcessorParameter* connectedParam = nullptr;
    Parameter* connectedParameter;

    ParameterPort::Ptr internalPort;
    ParameterPort::Ptr externalPort;

    bool parentIsInEditMode = false;
    bool openMode = false;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameter)
};

class SliderParameter : public Parameter
{
public:
    SliderParameter(AudioProcessorParameter* param = nullptr);

    void connect(Parameter* otherParameter) override;
    void disconnect(Parameter* otherParameter) override;

    void setEditMode(bool isEditable) override;
    void setParentEditMode(bool parentIsInEditMode) override;

    void setParameterValueAsync(float value) override;

    void setValueSync(float value) override;

private:
    SliderComponent slider;
    SliderListener listener;
};

class ComboParameter : public Parameter
{
public:
    ComboParameter(AudioProcessorParameter* param = nullptr);

    void connect(Parameter* otherParameter) override;
    void disconnect(Parameter* otherParameter) override;

    void setEditMode(bool isEditable) override;
    void setParentEditMode(bool parentIsInEditMode) override;
    
    void setParameterValueAsync(float value) override;
    void setValueSync(float value) override;


private:
    ComboComponent combo;
    ComboListener listener;
};


class ButtonParameter : public Parameter
{
public:
    ButtonParameter(AudioProcessorParameter* param = nullptr);

    void connect(Parameter* otherParameter) override;
    void disconnect(Parameter* otherParameter) override;

    void setEditMode(bool isEditable) override;
    void setParentEditMode(bool parentIsInEditMode) override;

    void setParameterValueAsync(float value) override;
    void setValueSync(float value) override;

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
