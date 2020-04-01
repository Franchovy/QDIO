/*
  ==============================================================================

    Effector.h
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/


#pragma once

#include "JuceHeader.h"


//======================================================================================
// Names for referencing EffectValueTree
const Identifier ID_TREE_TOP("TreeTop");
const Identifier ID_EFFECT_VT("effectTree");
const Identifier ID_EVT_OBJECT("Effect");
const Identifier ID_EFFECT_NAME("Name");
const Identifier ID_EFFECT_GUI("GUI");
// Ref for dragline VT
const Identifier ID_DRAGLINE("DragLine");

struct GUIEffect;
struct ConnectionLine;
struct LineComponent;

class CustomMenuItems
{
public:
    CustomMenuItems() = default;

    int addItem(String name, std::function<void()> function) {
        functions.add(function);
        names.add(name);
        return functions.size();
    }

    int addToMenu(PopupMenu &menu) {
        int numItems = menu.getNumItems();
        thisRange.setStart(numItems);
        for (int i = 0; i < names.size(); i++)
            menu.addItem(numItems++, names[i]);
        thisRange.setEnd(numItems);
        return numItems;
    }

    int execute(int result) {
        if (thisRange.contains(result))
            functions[result - thisRange.getStart()]();
    }

    int getSize() { return names.size(); }
    bool inRange(int i) { return thisRange.contains(i); }

private:
    Range<int> thisRange;
    Array<String> names;
    Array<std::function<void()>> functions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomMenuItems)
};

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
    void mouseDrag(const MouseEvent &event) override;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Resizer)
};

/**
 * Base class - port to connect to other ports
 */
class ConnectionPort : public Component
{
public:
    Point<int> centrePoint;
    bool hoverMode = false;

    void paint(Graphics &g) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void mouseEnter(const MouseEvent &event) override {
        hoverMode = true;
        repaint();
    }
    void mouseExit(const MouseEvent &event) override {
        hoverMode = false;
        repaint();
    }

    GUIEffect* getParent();

    virtual bool canConnect(ConnectionPort* other) = 0;

    bool isInput;
    ConnectionLine* connectionLine = nullptr;

protected:
    ConnectionPort() : Component() {}

    Rectangle<int> hoverBox;
    Rectangle<int> outline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionPort)
};

class AudioPort;
class InternalConnectionPort : public ConnectionPort
{
public:
    InternalConnectionPort(AudioPort* parent, bool isInput) : ConnectionPort() {
        audioPort = parent;
        this->isInput = isInput;

        hoverBox = Rectangle<int>(0,0,30,30);
        outline = Rectangle<int>(10,10,10,10);
        centrePoint = Point<int>(15,15);
        setBounds(0,0,30, 30);
    }

    bool canConnect(ConnectionPort *other) override;
    AudioPort* audioPort;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalConnectionPort)
};

class AudioPort : public ConnectionPort
{
public:
    explicit AudioPort(bool isInput);

    bool hitTest(int x, int y) override {
        return hoverBox.contains(x,y);
    }

    const InternalConnectionPort* getInternalConnectionPort() const { return &internalPort; }

    AudioProcessor::Bus* bus;
    InternalConnectionPort internalPort;

    bool canConnect(ConnectionPort *other) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPort)
};

struct ConnectionLine : public Component, public ComponentListener, public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<ConnectionLine>;

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;

    ConnectionLine(ConnectionPort& p1, ConnectionPort& p2);
    ~ConnectionLine(){
        inPort->connectionLine = nullptr;
        outPort->connectionLine = nullptr;
        inPort->removeComponentListener(this);
        outPort->removeComponentListener(this);
    }

    void paint(Graphics &g) override
    {
        int thiccness;
        if (hoverMode) {
            g.setColour(Colours::blue);
            thiccness = 3;
        } else {
            g.setColour(Colours::black);
            thiccness = 2;
        }

        g.drawLine(line.toFloat(),thiccness);
    }

    bool hitTest(int x, int y) override;

    void mouseEnter(const MouseEvent &event) override {
        hoverMode = true;
        repaint();
    }

    void mouseExit(const MouseEvent &event) override {
        hoverMode = false;
        repaint();
    }

    ConnectionPort* getOtherPort(ConnectionPort* port) {
        return inPort == port ? outPort : inPort;
    }

    bool hoverMode = false;

    ConnectionPort* inPort;
    ConnectionPort* outPort;
private:
    Line<int> line;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionLine)
};


struct LineComponent : public Component
{
    LineComponent() : dragLineTree(ID_DRAGLINE)
    {
        setBounds(0,0,getParentWidth(), getParentHeight());
        LineComponent::dragLine = this;
    }

    void resized() override {

    }

    void paint(Graphics &g) override {
        g.setColour(Colours::navajowhite);
        Path p;
        p.addLineSegment(line.toFloat(),1);
        PathStrokeType strokeType(1);

        float thiccness[] = {5, 5};
        strokeType.createDashedStroke(p, p, thiccness, 2);

        g.strokePath(p, strokeType);
    }

    /**
     * Updates dragLineTree connection property if connection is successful
     * @param port2
     */
    void convert(ConnectionPort* port2);

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
    ConnectionPort* port1 = nullptr;
    ValueTree dragLineTree;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LineComponent)
};


// TODO single child instead of children - wraps only around one thing
class GUIWrapper : public Component {
public:
    GUIWrapper(Component* child) : GUIWrapper()
    {
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
    void mouseUp(const MouseEvent& event) override {
        event.withNewPosition(event.getPosition() + getPosition());
        getParentComponent()->mouseUp(event);
    }

    PopupMenu& getMenu(){
        return menu;
    }

    //TODO do as clang says
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

    void moved() override {
        for (auto i : childComponents){
            i->moved();
        }
        Component::moved();
    }

    Component* getChild(){
        return childComponents.getFirst();
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIWrapper)
};

class ButtonListener : public Button::Listener
{
public:
    ButtonListener(AudioProcessorParameter* parameter) : Button::Listener() {
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
    ComboListener(AudioProcessorParameter* parameter) : ComboBox::Listener(){
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
    SliderListener(AudioProcessorParameter* parameter) : Slider::Listener(){
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

class EffectVT;

/**
 * Used for drag operations - small amount of data for undoable action
 */
struct EffectDragData : ReferenceCountedObject
{
    Point<int> previousPos;
    Component* previousParent;
};

/**
    GUIEffect Component
    GUI Representation of Effects / Container for plugins

    v-- Is this necessary??
    ReferenceCountedObject for usage as part of ValueTree system
*/
class GUIEffect : public ReferenceCountedObject, public Component {
public:
    GUIEffect (const MouseEvent &event, EffectVT* parentEVT);

    void childrenChanged() override;

    ~GUIEffect() override;

    void parentHierarchyChanged() override;

    void insertEffectGroup();
    void insertEffect();
    void setProcessor(AudioProcessor* processor);

    using Ptr = ReferenceCountedObjectPtr<GUIEffect>;

    void paint (Graphics& g) override;

    void resized() override;
    void moved() override;

    ConnectionPort* checkPort(Point<int> pos);

    void setParameters(const AudioProcessorParameterGroup* group);
    void addParameter(AudioProcessorParameter* param);

    void addPort(AudioProcessor::Bus* bus, bool isInput){
        auto p = isInput ?
                inputPorts.add(std::make_unique<AudioPort>(isInput)) :
                outputPorts.add(std::make_unique<AudioPort>(isInput));
        p->bus = bus;
        addAndMakeVisible(p);

        resized();


        if (!individual) {
            addChildComponent(p->internalPort);
            Point<int> d;
            d = isInput ? Point<int>(50, 0) : Point<int>(-50, 0);

            p->internalPort.setCentrePosition(getLocalPoint(p, p->centrePoint + d));
            p->internalPort.setVisible(editMode);
        } else {
            p->internalPort.setVisible(false);
        }
    }


    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    Point<int> dragDetachFromParentComponent();

    bool isIndividual() const { return individual; }
    bool hasBeenInitialised = false;

    EffectVT* EVT;

    bool hoverMode = false;
    bool selectMode = false;

    void setEditMode(bool isEditMode);
    bool toggleEditMode() { setEditMode(!editMode); }
    bool isInEditMode() { return editMode; }

    CustomMenuItems& getMenu() { return editMode ? editMenu : menu; }

private:
    bool individual = false;
    bool editMode = false;
    OwnedArray<AudioPort> inputPorts;
    OwnedArray<AudioPort> outputPorts;
    Label title;
    Image image;

    Resizer resizer;
    ComponentDragger dragger;
    ComponentBoundsConstrainer constrainer;

    CustomMenuItems menu;
    CustomMenuItems editMenu;

    Component* currentParent = nullptr;
    EffectDragData dragData;

    const AudioProcessorParameterGroup* parameters;

    //============================================================================
    // GUI auto stuff

    int portIncrement = 50;
    int inputPortStartPos = 100;
    int inputPortPos = inputPortStartPos;
    int outputPortStartPos = 100;
    int outputPortPos = outputPortStartPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIEffect)
};


/**
 * EffectVT (ValueTree) is an encapsulator for individual or combinations of AudioProcessors
 *
 * This includes GUI, AudioProcessor, and the Effect's ValueTree itself.
 *
 * The valuetree data structure is itself owned by this object, and has a property reference to the owner object.
 */

class EffectVT : public ReferenceCountedObject
{
public:
    /**
     * ENCAPSULATOR CONSTRUCTOR EffectVT of group of EffectVTs
     * @param effectVTSet
     */
    EffectVT(const MouseEvent &event, Array<const EffectVT*> effectVTSet) :
        EffectVT(event)
    {
        // Note top left and bottom right effects to have a size to set
        Point<int> topLeft;
        Point<int> bottomRight;
        auto thisBounds = guiEffect.getBoundsInParent();

        for (auto eVT : effectVTSet){
            // Set itself as parent of given children
            eVT->getTree().getParent().removeChild(eVT->getTree(), nullptr);
            effectTree.appendChild(eVT->getTree(), nullptr);

            // Update position
            auto bounds = eVT->getGUIEffect()->getBoundsInParent();
            thisBounds = thisBounds.getUnion(bounds);
        }

        thisBounds.expand(10,10);
        guiEffect.setBounds(thisBounds);
    }

    /**
     * INDIVIDUAL CONSTRUCTOR
     * Node - Individual GUIEffect / effectVT
     * @param nodeID
     */
    EffectVT(const MouseEvent &event, AudioProcessorGraph::NodeID nodeID) :
        EffectVT(event)
    {
        // Create from node:
        node = graph->getNodeForId(nodeID);
        processor = node->getProcessor();
        effectTree.setProperty("Node", node.get(), nullptr);

        // Initialise with processor
        guiEffect.setProcessor(processor);
    }

    /**
     * Empty effectVT
     */
    EffectVT(const MouseEvent &event) :
        effectTree("effectTree"),
        guiEffect(event, this)
    {
        // Setup effectTree properties
        effectTree.setProperty("Name", name, nullptr);
        effectTree.setProperty("Effect", this, nullptr);
        effectTree.setProperty("GUI", &guiEffect, nullptr);
    }

    ~EffectVT()
    {
        effectTree.removeAllProperties(nullptr);
        // Delete processor from graph
        graph->removeNode(node->nodeID);
    }

    using Ptr = ReferenceCountedObjectPtr<EffectVT>;

    /**
     * Adds effect to this one as a child
     * @param effect to add as child
     */
    void addEffect(const EffectVT::Ptr effect){
        std::cout << "Is this still being called?" << newLine;
        effect->getTree().getParent().removeChild(effect->getTree(), nullptr);
        effectTree.appendChild(effect->getTree(), nullptr);
    }

    void addConnection(ConnectionLine* connection) {
        connections.add(connection);
        guiEffect.addAndMakeVisible(connection);
    }

    // =================================================================================
    // Setters and getter functions

    struct NodeAndPort {
        AudioProcessorGraph::Node::Ptr node = nullptr;
        AudioPort* port = nullptr;
        bool isValid = false;
    };

    NodeAndPort getNode(ConnectionPort* port) {
        NodeAndPort nodeAndPort;

        if (port->connectionLine == nullptr)
            return nodeAndPort;
        else if (isIndividual()) {
            nodeAndPort.node = node;
            nodeAndPort.port = dynamic_cast<AudioPort *>(port);
            nodeAndPort.isValid = true;
            return nodeAndPort;
        } else {
            ConnectionPort* portToCheck = nullptr;
            if (auto p = dynamic_cast<AudioPort*>(port))
                portToCheck = &p->internalPort;
            else if (auto p = dynamic_cast<InternalConnectionPort*>(port))
                portToCheck = p->audioPort;

            if (portToCheck->connectionLine != nullptr) {
                 auto otherPort = portToCheck->connectionLine->getOtherPort(portToCheck);
                 nodeAndPort = otherPort->getParent()->EVT->getNode(
                        otherPort);
                return nodeAndPort;
            } else return nodeAndPort;
        }
    }

    AudioProcessorGraph::NodeID getNodeID() const {
        return node->nodeID;
    }

    // Data
    const String& getName() const { return name; }
    void setName(const String &name) { this->name = name; }
    ValueTree& getTree() {return effectTree;}
    const ValueTree& getTree() const {return effectTree;}
    GUIEffect* getGUIEffect() {return &guiEffect;}
    const GUIEffect* getGUIEffect() const {return &guiEffect;}
    static void setAudioProcessorGraph(AudioProcessorGraph* processorGraph) {graph = processorGraph;}
    Array<ConnectionLine*> getConnections() const {
        Array<ConnectionLine*> array;
        for (auto c : connections){
            array.add(c);
        }
        return array;
    }

    AudioProcessor::Bus* getDefaultBus() { graph->getBus(true, 0); }

    // Convenience functions
    EffectVT::Ptr getParent(){ return dynamic_cast<EffectVT*>(
            effectTree.getParent().getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
    EffectVT::Ptr getChild(int index){ return dynamic_cast<EffectVT*>(
            effectTree.getChild(index).getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
    Array<EffectVT::Ptr> getChildren() {
        Array<EffectVT::Ptr> array;
        for (int i = 0; effectTree.getNumChildren(); i++)
            if (auto e = dynamic_cast<EffectVT*>(effectTree.getChild(i).getProperty(ID_EVT_OBJECT).getObject()))
                array.add(e);
        return array;
    }
    EffectVT::Ptr ptr(){ return this; }
    int getNumChildren(){ return effectTree.getNumChildren(); }

    bool isIndividual() const { return guiEffect.isIndividual(); }

private:
    // Used for an individual processor EffectVT. - does not contain anything else
    AudioProcessor* processor = nullptr;
    AudioProcessorGraph::Node::Ptr node = nullptr;

    ReferenceCountedArray<ConnectionLine> connections;

    String name;
    ValueTree effectTree;
    GUIEffect guiEffect;

    static AudioProcessorGraph* graph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectVT)

};
