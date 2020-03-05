/*
  ==============================================================================

    Effector.h
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"


class Resizer : public Component
{
public:
    Resizer() :
            strokeType(1.f)
    {
        setSize(30,30);
        box.setSize(getWidth(), getHeight());
    }

    void paint(Graphics& g) override {
        //g.drawRect(box);
    }

    void mouseDown(const MouseEvent &event) override{
        startPos = Point<float>(getX() + event.getMouseDownX(), getY() + event.getMouseDownY());
        dragger.startDraggingComponent(this, event);
        setMouseCursor(MouseCursor::DraggingHandCursor);
        Component::mouseDown(event);
    }
    void mouseDrag(const MouseEvent &event) override{
        dragger.dragComponent(this, event, nullptr);

        getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                      startPos.y + event.getDistanceFromDragStartY());
        Component::mouseDrag(event);
    }
    void mouseUp(const MouseEvent &event) override{
        setMouseCursor(MouseCursor::ParentCursor);
        Component::mouseUp(event);
    }

    void parentHierarchyChanged() override {
        setCentrePosition(getParentComponent()->getWidth(), getParentComponent()->getHeight());
        Component::parentHierarchyChanged();
    }

    ~Resizer() override
    {
    }

    void parentSizeChanged() override {
        setCentrePosition(getParentWidth(), getParentHeight());
        Component::parentSizeChanged();
    }

    void mouseEnter(const MouseEvent &event) override {
        setMouseCursor(MouseCursor::TopLeftCornerResizeCursor);
        Component::mouseEnter(event);
    }

    void mouseExit(const MouseEvent &event) override {
        setMouseCursor(getParentComponent()->getMouseCursor());
        Component::mouseExit(event);
    }

private:
    Point<float> startPos;
    Rectangle<float> box;
    ComponentDragger dragger;

    MouseCursor prevMouseCursor;

    PathStrokeType strokeType;
    float dashLengths[2] = {1.f, 1.f};

};


class GUIWrapper : public Component
{
public:
    GUIWrapper(Component* child){
        GUIWrapper();
        addAndMakeVisible(child);
    }

    GUIWrapper() {
        setSize(800,800);
        size.setXY(getWidth(), getHeight());
        outline.setBounds(0,0,getWidth(),getHeight());

        addChildComponent(resizer);
        resizer.setAlwaysOnTop(true);

        closeButton.setButtonText("Close");
        addChildComponent(closeButton);
        closeButton.onClick = [=]{
            setVisible(false);
        };

        addChildComponent(title);
    }

    void paint (Graphics& g) override {
        g.drawRect(outline);
    }
    void resized() override {

        title.setBounds(size.x/2 - 30, 10, 80, 30);
        closeButton.setBounds(size.x - 80, 10, 70, 30);
        outline.setBounds(0,0,size.x,size.y);
        for (auto c : childComponents)
            c->setBounds(30,30,size.x-30,size.y-30);
        size.setXY(getWidth(),getHeight());
    }

    void mouseDown(const MouseEvent& event) override {
        dragger.startDraggingComponent(this, event);
        if (event.mods.isRightButtonDown())
            getParentComponent()->mouseDown(event);
        Component::mouseDown(event);
    }
    void mouseDrag(const MouseEvent& event) override {
        dragger.dragComponent(this, event, nullptr);
    }

    PopupMenu& getMenu(){
        return menu;
    }

    void addToMenu(String item){
        menu.addItem(item);
    }

    void setTitle(String name){
        title.setText(name,dontSendNotification);
    }

    void setVisible(bool shouldBeVisible) override {
        closeButton.setVisible(shouldBeVisible);
        resizer.setVisible(shouldBeVisible);
        title.setVisible(shouldBeVisible);
        for (auto c : childComponents){
            c->setVisible(shouldBeVisible);
        }
        Component::setVisible(shouldBeVisible);
    }

    void childrenChanged() override {
        childComponents.clear();
        for (auto c : getChildren()){
            if (c != &resizer && c != &closeButton && c != &title)
                childComponents.add(c);
        }
        Component::childrenChanged();
        resized();
    }

    ~GUIWrapper() override {
        childComponents.clear();
    }

    void parentSizeChanged() override {
        // Keep size
        setSize(size.x, size.y);
    }

    TextButton closeButton;

private:
    Point<int> size; // unchanging size
    Label title;
    Rectangle<float> outline;
    ComponentDragger dragger;
    PopupMenu menu;
    Resizer resizer;
    Array<Component*> childComponents;
};


typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

class EffectProcessorEditor : public AudioProcessorEditor
{
public:
    EffectProcessorEditor (AudioProcessor& parent, AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (parent),
    valueTreeState (vts)
    {
        gainLabel.setText ("Gain", dontSendNotification);
        addAndMakeVisible (gainLabel);

        addAndMakeVisible (gainSlider);
        gainAttachment.reset (new SliderAttachment (valueTreeState, "gain", gainSlider));

        this->setResizable(true,true);
        setSize (300, 300);
    }
private:
    AudioProcessorValueTreeState& valueTreeState;

    Label gainLabel;
    Slider gainSlider;
    std::unique_ptr<SliderAttachment> gainAttachment;
};

class EffectProcessor : public AudioProcessor
{
public:
    EffectProcessor() : parameters (*this, nullptr, Identifier ("APVTSTutorial"),
                                    {std::make_unique<AudioParameterFloat> ("gain", "Gain", 0.0f, 1.0f, 0.9f)})
    {
        // Note: base class AudioProcessor takes ownership of parameters.
        levelParameter = parameters.getRawParameterValue ("gain");
    }

    //===============================================================================
    // Audio Processing
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
    /*    setPlayConfigDetails (getMainBusNumInputChannels(),
                              getMainBusNumOutputChannels(),
                              sampleRate,
                              maximumExpectedSamplesPerBlock);*/
    }
    void releaseResources() override {}
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override {
        buffer.applyGain(*levelParameter);
    }

    //===============================================================================
    // Function overrides - all the default crap

    const String getName() const override { return "haga baga chook chook"; }
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    AudioProcessorEditor *createEditor() override { return new GenericAudioProcessorEditor(*this);}//new EffectProcessorEditor (*this, parameters); }
    bool hasEditor() const override { return true; }
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const String getProgramName(int index) override { return String(); }
    void changeProgramName(int index, const String &newName) override {}

    //===============================================================================
    // Set and Load entire plugin state

    void getStateInformation(juce::MemoryBlock &destData) override {
        auto state = parameters.copyState();
        std::unique_ptr<XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }

    void setStateInformation(const void *data, int sizeInBytes) override {
        std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName (parameters.state.getType()))
                parameters.replaceState (ValueTree::fromXml (*xmlState));
    }

private:
    // You can store the VT elsewhere but:
    // - it can only be used for this one processor
    // - it must have the same lifetime as this processor
    AudioProcessorValueTreeState parameters;
    std::atomic<float>* levelParameter  = nullptr;
};

/**
    GUIEffect Component
    GUI Representation of Effects / Container for plugins
    ReferenceCountedObject for usage as part of ValueTree system
*/
class GUIEffect  : public Component, public ReferenceCountedObject
{
public:
    GUIEffect ();
    ~GUIEffect() override;

    using Ptr = ReferenceCountedObjectPtr<GUIEffect>;

    void paint (Graphics& g) override;
    void resized() override;

    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;


private:
    Rectangle<float> outline;
    Resizer resizer;

    ComponentDragger dragger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIEffect)
};

/**
   EffectVT (ValueTree) is the manager of everything in an Effect.

   This includes GUI, AudioProcessor, and the Effect's ValueTree itself.

   It's a little confusing because the ValueTree owned by the object goes on to refer to this object - but the
   destructor should take care of that.
 */


class EffectVT : public ReferenceCountedObject
{
public:
    explicit EffectVT(AudioProcessorGraph* graph) : effectTree("effectTree")
    {
        auto node = graph->addNode(std::make_unique<EffectProcessor>());
        processor = node->getProcessor();

        // Set this object as a property of the owned valuetree....
        effectTree.setProperty("effect", this, nullptr);
        // Set GUI object as property
        effectTree.setProperty("GUI", &gui, nullptr);
        // Create AudioProcessor
        // default processor
    }
    ~EffectVT()
    {
        effectTree.removeAllProperties(nullptr);
        // Delete processor from graph
        // Delete processorVT
    }

    using Ptr = ReferenceCountedObjectPtr<EffectVT>;

    ValueTree& getTree() {return effectTree;}

private:
    ValueTree effectTree;
    GUIEffect gui;

    AudioProcessorGraph* graph;
    AudioProcessor* processor;
    AudioProcessorValueTreeState processorVT;

};
