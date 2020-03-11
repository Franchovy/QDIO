/*
  ==============================================================================

    Effector.h
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/


#pragma once

#include "JuceHeader.h"



struct ConnectionLine;
struct LineComponent;



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



struct ConnectionPort : public Component
{
    ConnectionPort(bool isInput) : rectangle(20,20)
    {
        setBounds(rectangle);
        this->isInput = isInput;
    }

    void paint(Graphics &g) override
    {
        g.setColour(Colours::black);
        //rectangle.setPosition(10,10);
        g.drawRect(rectangle,2);
    }

    void connect(ConnectionPort& otherPort);

    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;

    void mouseUp(const MouseEvent &event) override;

    /**
     * @return Position of parent relative to maincomponent
     */
    Point<int> getMainParentPos(){
        auto p = getParentComponent();
        auto pos = Point<int>(0,0);
        while (p->getName() != "QDIO"){
            pos += p->getPosition();
            p = p->getParentComponent();
        }
        return pos;
    }

    /**
     * @return centre position relative to maincomponent
     */
    Point<int> getMainCentrePos(){
        auto pos = getMainParentPos();
        pos += getPosition();
        pos += Point<int>(getWidth()/2, getHeight()/2);
        return pos;
    }

    bool isInput;
    ConnectionLine* line;
    Rectangle<int> rectangle;
    AudioChannelSet* bus;
};

struct ConnectionLine : public Component, public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<ConnectionLine>;

    ConnectionLine(ConnectionPort& p1, ConnectionPort& p2){
        if (p1.isInput) {
            inPort = &p1;
            outPort = &p2;
        } else {
            inPort = &p2;
            outPort = &p1;
        }
    }

    void paint(Graphics &g) override
    {
        Line<float> line(inPort->getPosition().toFloat(), outPort->getPosition().toFloat());
        g.drawLine(line,2);
    }

    ConnectionPort* inPort;
    ConnectionPort* outPort;

private:
    Line<float> line;
};


struct LineComponent : public Component
{
    LineComponent() : dragLineTree("DragLine")
    {
        setBounds(0,0,getParentWidth(), getParentHeight());
        LineComponent::dragLine = this;
    }

    void resized() override {

    }

    void paint(Graphics &g) override {
        g.drawLine(line.toFloat());
    }

    ConnectionLine::Ptr convert(ConnectionPort* p2){
        return new ConnectionLine(*port1, *p2);
    }

    ConnectionLine::Ptr lastConnectionLine;

    ValueTree getDragLineTree() {return dragLineTree;}

    static LineComponent* getDragLine(){
        return dragLine;
    }

    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;

    void mouseUp(const MouseEvent &event) override;

private:
    static LineComponent* dragLine;

    Line<int> line;
    Point<int> p1, p2;
    ConnectionPort* port1;
    ValueTree dragLineTree;
};



class GUIWrapper : public Component {
public:
    GUIWrapper(Component* child){
        GUIWrapper();
        addAndMakeVisible(child);
    }

    GUIWrapper(bool closeButtonEnabled = false) {
        setSize(100,100);

        size.setXY(getWidth(), getHeight());
        outline.setBounds(0,0,getWidth(),getHeight());
        setBounds(outline);

        addAndMakeVisible(resizer);
        resizer.setAlwaysOnTop(true);

        if (closeButtonEnabled) {
            closeButton.setButtonText("Close");
            addAndMakeVisible(closeButton);
            closeButton.onClick = [=] {
                setVisible(false);
            };
            closeButton.setAlwaysOnTop(true);
        }
    }

    void paint (Graphics& g) override {
        g.fillAll(Colours::white);
        g.setColour(Colours::black);
        g.drawRect(outline);
        g.drawText(title, 10,10,100,40,Justification::left);
        //Component::paint(g);
    }
    void resized() override {
        size.setXY(getWidth(),getHeight());
        closeButton.setBounds(size.x - 80, 10, 70, 30);
        outline.setBounds(0,0,size.x,size.y);
        for (auto c : childComponents)
            c->setBounds(10,10,getWidth()-10,getHeight()-10);

        repaint();
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
        title = name;
    }

    void setVisible(bool shouldBeVisible) override {
        for (auto c : childComponents){
            c->setVisible(shouldBeVisible);
        }
        Component::setVisible(shouldBeVisible);
    }

    void childrenChanged() override {
        childComponents.clear();
        for (auto c : getChildren()){
            if (c != &resizer && c != &closeButton) {
                childComponents.add(c);
            }
        }
        // If this is the first child to be added, adjust to its size
        if (childComponents.size() == 1){
            setSize(childComponents.getFirst()->getWidth(),childComponents.getFirst()->getHeight());
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
    String title;
    Rectangle<int> outline;
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
/*

    EffectProcessor(AudioProcessorValueTreeState& parameters)
    {
        this->parameters = parameters;
        // Note: base class AudioProcessor takes ownership of parameters.
        levelParameter = parameters.getRawParameterValue ("gain");
    }
*/

    //===============================================================================
    // Audio Processing
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
    /*    setPlayConfigDetails (getMainBusNumInputChannels(),
                              getMainBusNumOutputChannels(),
                              sampleRate,
                              maximumExpectedSamplesPerBlock);*/
    }
    void releaseResources() override {

    }
    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages) override {
        for (auto c = getMainBusNumInputChannels(); c < getMainBusNumOutputChannels(); c++){
            buffer.clear(c, 0, buffer.getNumSamples());
        }

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

class EffectVT;

/**
    GUIEffect Component
    GUI Representation of Effects / Container for plugins

    v-- Is this necessary??
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

    void setParameters(AudioProcessorParameterGroup& group);

    void addPort(ConnectionPort* p){
        if (p->isInput) {
            addAndMakeVisible(p);
            inputPorts.add(p);
        } else {
            addAndMakeVisible(p);
            outputPorts.add(p);
        }
        resized();
    }

    void mouseDown(const MouseEvent &event) override;

    void mouseDrag(const MouseEvent &event) override;

private:
    Array<ConnectionPort*> inputPorts;
    Array<ConnectionPort*> outputPorts;
    int portIncrement = 30;
    int inputPortStartPos = 50;
    int inputPortPos = inputPortStartPos;
    int outputPortStartPos = 50;
    int outputPortPos = outputPortStartPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIEffect)
};

/**
   EffectVT (ValueTree) is an encapsulator for individual or combinations of AudioProcessors

   This includes GUI, AudioProcessor, and the Effect's ValueTree itself.

   It's a little confusing because the ValueTree owned by the object goes on to refer to this object - but the
   destructor should take care of that.
 */

class EffectVT : public ReferenceCountedObject
{
public:
/**
     * EffectVT - encapsulator of individual AudioProcessor
     * This is used to contain BaseEffects, imported Plugins, or AudioGraphIOProcessors
     * Initialises all data to be used as an individual component, and for further up trees.
     */
    /*template <class ProcessorType>
    static EffectVT create(AudioProcessorGraph* graph, String name = "") {
        // Creates Processor of given type
        auto node = graph->addNode(std::make_unique<ProcessorType>());
        auto nodeID = node->nodeID;

        return EffectVT(nodeID, graph, name);
    }*/

    /**
     * Constructor for a set of EffectVTs
     */
     EffectVT(Array<EffectVT> effectVTSet, AudioProcessorGraph* graph, String name = "") {
         // TODO
     }

    /**
     * INDIVIDUAL: Constructor for base effects, plugins, etc. Create, pass to the audio graph, and then pass nodeID
     * to this constructor.
     * @param nodeID
     * @param graph
     * @param name
     */
    EffectVT(AudioProcessorGraph::NodeID nodeID, AudioProcessorGraph* graph, String name = "") :
            EffectVT(graph, name)
{
        //std::cout << "Registering processor from node " << newLine;
        // Register the processor and node
        node = graph->getNodeForId(nodeID);
        processor = node->getProcessor();
        isIndividual = true;
        // Set node property
        effectTree.setProperty("Node", node.get(), nullptr);

        gui.addAndMakeVisible(guiEffect);
        for (auto inB : processor->getBusesLayout().inputBuses){
            auto p = addPort(true);
            p->bus = &inB;
            guiEffect.addPort(p);
        }
        for (auto outB : processor->getBusesLayout().outputBuses){
            auto p = addPort(false);
            p->bus = &outB;
            guiEffect.addPort(p);
        }

        // TODO Create Default Ports based on Processor Bus Layout
    }

    /**
     * Constructor for an empty effect. It doesn't contain anything but can receive contents
     * upon drag and drop into it.
     * @param name
     * @param graph
     */
    EffectVT(AudioProcessorGraph* graph, String name = "")
            : effectTree("effectTree")
    {
        // Set name
        if (name.isNotEmpty())
            this->name = name;
        else name = "Default name";

        gui.setTitle(name);

        // Set name property
        effectTree.setProperty("Name", name, nullptr);
        // Set "self" property
        effectTree.setProperty("Effect", this, nullptr);
        // Set GUI property
        //effectTree.setProperty("GUI", , nullptr);
    }

    ~EffectVT()
    {
        effectTree.removeAllProperties(nullptr);
        // Delete processor from graph
        graph->removeNode(node->nodeID);
    }

    using Ptr = ReferenceCountedObjectPtr<EffectVT>;

    const String& getName() const { return name; }
    void setName(const String &name) { this->name = name; }
    const ValueTree& getTree() const {return effectTree;}

    GUIWrapper* getGUIWrapper() {return &gui;}

    // =================================================================================
    // Individual data
    const AudioProcessor* getProcessor() const {return processor;}
    const AudioProcessorGraph::Node::Ptr getNode() const {return node;}
    const GUIEffect* getGUIEffect() {return &guiEffect;}

    ConnectionPort* addPort(bool isInput){
        return ports.add(std::make_unique<ConnectionPort>(isInput));
    }

private:
    // Used for an individual processor EffectVT. - does not contain anything else
    bool isIndividual = false;
    AudioProcessor* processor;
    AudioProcessorGraph::Node::Ptr node;
    GUIEffect guiEffect;

    String name;
    ValueTree effectTree;
    GUIWrapper gui;

    OwnedArray<ConnectionPort> ports;

    AudioProcessorParameterGroup parameters;

    AudioProcessorGraph* graph;

};
