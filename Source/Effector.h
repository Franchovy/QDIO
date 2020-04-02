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

struct GuiEffect;
struct ConnectionLine;
struct LineComponent;

class GuiObject : public ReferenceCountedObject, public Component
{
public:
    using Ptr = ReferenceCountedObjectPtr<GuiObject>;
};

class CustomMenuItems
{
public:
    CustomMenuItems() = default;
    ~CustomMenuItems() {
        // TODO smart pointer to manage - deallocate here
        /*names.clear();
        functions.clear();*/
    }

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

class Resizer : public GuiObject
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
class ConnectionPort : public GuiObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ConnectionPort>;

    ~ConnectionPort(){

    }

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

    GuiEffect* getParent();

    virtual bool canConnect(ConnectionPort::Ptr& other) = 0;

    bool isInput;
    ConnectionLine* connectionLine = nullptr;

protected:
    ConnectionPort() : GuiObject() {}

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

    bool canConnect(ConnectionPort::Ptr& other) override;
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

    const InternalConnectionPort::Ptr getInternalConnectionPort() const { return internalPort; }

    AudioProcessor::Bus* bus;
    InternalConnectionPort::Ptr internalPort;

    bool canConnect(ConnectionPort::Ptr& other) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPort)
};

struct ConnectionLine : public GuiObject, public ComponentListener
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

    ConnectionPort::Ptr getOtherPort(ConnectionPort::Ptr& port) {
        return inPort == port ? outPort : inPort;
    }

    bool hoverMode = false;

    ConnectionPort::Ptr inPort;
    ConnectionPort::Ptr outPort;
private:
    Line<int> line;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionLine)
};


struct LineComponent : public GuiObject
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
class GUIWrapper : public GuiObject {
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

class GuiParameter : public GuiObject
{

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
    GuiEffect Component
    GUI Representation of Effects / Container for plugins

    v-- Is this necessary??
    ReferenceCountedObject for usage as part of ValueTree system
*/
class GuiEffect : public GuiObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GuiEffect>;

    GuiEffect (const MouseEvent &event, EffectVT* parentEVT);
    ~GuiEffect() override;

    void setProcessor(AudioProcessor* processor);

    void paint (Graphics& g) override;
    void resized() override;
    void moved() override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    void childrenChanged() override;
    void parentHierarchyChanged() override;

    ConnectionPort::Ptr checkPort(Point<int> pos);

    void setParameters(const AudioProcessorParameterGroup* group);
    void addParameter(AudioProcessorParameter* param);

    void addPort(AudioProcessor::Bus* bus, bool isInput);

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuiEffect)
};

/**
 * Base class for Effects and EffectScene
 * Contains all functionality common to both:
 * Convenience functions for tree navigation,
 * Data for saving and loading state,
 * TODO Undoable action
 */
class EffectTreeBase : public ReferenceCountedObject
{
public:

private:

};

/**
 * EffectVT (ValueTree) is an encapsulator for individual or combinations of AudioProcessors
 *
 * This includes GUI, AudioProcessor, and the Effect's ValueTree itself.
 *
 * The valuetree data structure is itself owned by this object, and has a property reference to the owner object.
 */

class EffectVT : public EffectTreeBase
{
public:

    EffectVT(const MouseEvent &event, Array<const EffectVT*> effectVTSet);
    EffectVT(const MouseEvent &event, AudioProcessorGraph::NodeID nodeID);
    explicit EffectVT(const MouseEvent &event);

    ~EffectVT();

    using Ptr = ReferenceCountedObjectPtr<EffectVT>;

    void addConnection(ConnectionLine* connection);

    // =================================================================================
    // Setters and getter functions

    struct NodeAndPort {
        AudioProcessorGraph::Node::Ptr node = nullptr;
        AudioPort* port = nullptr;
        bool isValid = false;
    };
    NodeAndPort getNode(ConnectionPort::Ptr& port);
    AudioProcessorGraph::NodeID getNodeID() const;

    // Data
    const String& getName() const { return name; }
    void setName(const String &name) { this->name = name; }
    ValueTree& getTree() {return effectTree;}
    const ValueTree& getTree() const {return effectTree;}
    GuiEffect* getGUIEffect() {return &guiEffect;}
    const GuiEffect* getGUIEffect() const {return &guiEffect;}
    static void setAudioProcessorGraph(AudioProcessorGraph* processorGraph) {graph = processorGraph;}

    AudioProcessor::Bus* getDefaultBus() { graph->getBus(true, 0); }

    // Convenience functions
    EffectVT::Ptr getParent(){ return dynamic_cast<EffectVT*>(
            effectTree.getParent().getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
    EffectVT::Ptr getChild(int index){ return dynamic_cast<EffectVT*>(
            effectTree.getChild(index).getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
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
    GuiEffect guiEffect;

    static AudioProcessorGraph* graph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectVT)

};
