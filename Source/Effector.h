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

};



struct ConnectionPort : public Component
{
    ConnectionPort(bool isInput) : rectangle(20,20)
    {
        setBounds(rectangle.expanded(20));
        this->isInput = isInput;
    }

    void paint(Graphics &g) override
    {
        g.setColour(Colours::black);
        //rectangle.setPosition(10,10);
        g.drawRect(rectangle.withPosition(20,20),2);

        // Hover rectangle
        g.setColour(Colours::blue);
        Path hoverRectangle;
        hoverRectangle.addRoundedRectangle(0, 0, getWidth(), getHeight(), 10, 10);
        PathStrokeType strokeType(3);

        if (hoverMode) {
            float thiccness[] = {5, 7};
            strokeType.createDashedStroke(hoverRectangle, hoverRectangle, thiccness, 2);
            g.strokePath(hoverRectangle, strokeType);
        }
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

    void moved() override;

    void mouseEnter(const MouseEvent &event) override {
        hoverMode = true;
        repaint();
    }

    void mouseExit(const MouseEvent &event) override {
        hoverMode = false;
        repaint();
    }

    bool isInput;
    ConnectionLine* line = nullptr;
    Rectangle<int> rectangle;
    AudioProcessor::Bus* bus;
    bool hoverMode = false;
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
        inPort->line = this;
        outPort->line = this;

        line = Line<float>(inPort->getMainCentrePos().toFloat(),outPort->getMainCentrePos().toFloat());
        setBounds(0,0,getParentWidth(),getParentHeight());
    }

    void move(bool isInput, Point<float> newPoint){
        if (isInput)
            line.setStart(inPort->getMainCentrePos().toFloat());
        else
            line.setEnd(outPort->getMainCentrePos().toFloat());
        repaint();
    }

    void paint(Graphics &g) override
    {
        g.drawLine(line,2);
    }

    ~ConnectionLine() override {
        inPort->line = nullptr;
        outPort->line = nullptr;
    }

    ConnectionPort* inPort;
    ConnectionPort* outPort;

private:
    Line<float> line;
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
class GUIEffect  : public ReferenceCountedObject, public Component {
public:
    GUIEffect (const MouseEvent &event, EffectVT* parentEVT);

    void visibilityChanged() override;

    void childrenChanged() override;

    ~GUIEffect() override;

    void parentHierarchyChanged() override;

    void insertEffectGroup();
    void insertEffect();
    void setProcessor(AudioProcessor* processor);

    using Ptr = ReferenceCountedObjectPtr<GUIEffect>;

    void paint (Graphics& g) override;
    void resized() override;


    void setParameters(AudioProcessorParameterGroup& group);

    void addPort(AudioProcessor::Bus* bus, bool isInput){
        auto p = isInput ?
                inputPorts.add(std::make_unique<ConnectionPort>(isInput)) :
                outputPorts.add(std::make_unique<ConnectionPort>(isInput));
        p->bus = bus;
        addAndMakeVisible(p);
    }


    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    void moved() override;

    Point<int> dragDetachFromParentComponent();

    bool isIndividual() { return individual; }
    bool hasBeenInitialised = false;

    EffectVT* EVT;

    bool hoverMode = false;
    bool selectMode = false;

private:
    bool individual = false;
    OwnedArray<ConnectionPort> inputPorts;
    OwnedArray<ConnectionPort> outputPorts;

    Resizer resizer;
    ComponentDragger dragger;
    ComponentBoundsConstrainer constrainer;

    Component* currentParent = nullptr;
    EffectDragData dragData;

    AudioProcessorParameterGroup parameters; // dum dum dum

    //============================================================================
    // GUI auto stuff

    int portIncrement = 30;
    int inputPortStartPos = 50;
    int inputPortPos = inputPortStartPos;
    int outputPortStartPos = 50;
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
        isIndividual = true;

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

    // =================================================================================
    // Setters and getter functions

    // Individual data
    const AudioProcessor* getProcessor() const {if (isIndividual) return processor;}
    const AudioProcessorGraph::Node::Ptr getNode() const {if (isIndividual) return node;}

    // Data
    const String& getName() const { return name; }
    void setName(const String &name) { this->name = name; }
    ValueTree& getTree() {return effectTree;}
    const ValueTree& getTree() const {return effectTree;}
    GUIEffect* getGUIEffect() {return &guiEffect;}
    const GUIEffect* getGUIEffect() const {return &guiEffect;}
    static void setAudioProcessorGraph(AudioProcessorGraph* processorGraph) {graph = processorGraph;}

    // Convenience functions
    EffectVT::Ptr getParent(){ return dynamic_cast<EffectVT*>(
            effectTree.getParent().getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
    EffectVT::Ptr getChild(int index){ return dynamic_cast<EffectVT*>(
            effectTree.getChild(index).getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
    EffectVT::Ptr ptr(){ return this; }
    int getNumChildren(){ return effectTree.getNumChildren(); }

private:
    // Used for an individual processor EffectVT. - does not contain anything else
    bool isIndividual = false;
    AudioProcessor* processor = nullptr;
    AudioProcessorGraph::Node::Ptr node = nullptr;

    String name;
    ValueTree effectTree;
    GUIEffect guiEffect;

    static AudioProcessorGraph* graph;

};
