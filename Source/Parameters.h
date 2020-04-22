/*
  ==============================================================================

    Parameters.h
    Created: 23 Apr 2020 8:08:42am
    Author:  maxime

  ==============================================================================
*/

#pragma once


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

