/*
  ==============================================================================

    EffectGui.h
    Created: 5 Apr 2020 11:20:04am
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>

#pragma once

// Names for referencing EffectValueTree
const Identifier ID_TREE_TOP("TreeTop");
const Identifier ID_EFFECT_VT("tree");
const Identifier ID_EVT_OBJECT("Effect");
const Identifier ID_EFFECT_NAME("Name");
const Identifier ID_EFFECT_GUI("GUI");
// Ref for dragline VT
const Identifier ID_DRAGLINE("DragLine");

class GuiObject : public ReferenceCountedObject, public Component
{
public:
    using Ptr = ReferenceCountedObjectPtr<GuiObject>;

    GuiObject() = default;
    ~GuiObject() override {
        resetReferenceCount();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuiObject)
};

/**
 * SelectedItemSet for Component* class, with
 * itemSelected/itemDeselected overrides. That is all.
 */
class ComponentSelection : public SelectedItemSet<GuiObject::Ptr>
{
public:
    ComponentSelection() = default;
    ~ComponentSelection() = default;

    void clear() {
        SelectedItemSet<GuiObject::Ptr>::deselectAll();
    }

    void itemSelected(GuiObject::Ptr type) override;
    void itemDeselected(GuiObject::Ptr type) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentSelection)
};


class SelectHoverObject : public GuiObject
{
public:
    SelectHoverObject();
    ~SelectHoverObject();

    using Ptr = ReferenceCountedObjectPtr<SelectHoverObject>;

    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    static void setHoverComponent(SelectHoverObject* item);
    static void setHoverComponent(SelectHoverObject::Ptr item);
    static void resetHoverObject();

    static void addSelectObject(SelectHoverObject* item);
    static void removeSelectObject(SelectHoverObject* item);

    void setSelectMode(bool newSelectMode);

protected:
    static SelectHoverObject* hoverComponent;
    static ReferenceCountedArray<SelectHoverObject> componentsToSelect;
    static ComponentSelection selected;

    static void close();

    bool hoverMode = false;
    bool selectMode = false;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelectHoverObject)
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
        for (auto && name : names)
            menu.addItem(numItems++, name);
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
class ConnectionPort : public SelectHoverObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<ConnectionPort>;

    ~ConnectionPort() = default;

    Point<int> centrePoint;

    void paint(Graphics &g) override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    virtual bool canConnect(ConnectionPort::Ptr& other) = 0;

    bool isInput;

    void setOtherPort(ConnectionPort::Ptr newPort) { otherPort = newPort; }
    ConnectionPort::Ptr getOtherPort() { return otherPort; }

    bool isConnected() { return otherPort != nullptr; }

protected:
    ConnectionPort() = default;

    ConnectionPort::Ptr otherPort = nullptr;

    Rectangle<int> hoverBox;
    Rectangle<int> outline;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionPort)
};

class AudioPort;
class InternalConnectionPort : public ConnectionPort
{
public:
    void mouseDown(const MouseEvent &event) override;

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
private:
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

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPort)

    void mouseDown(const MouseEvent &event) override;
};

struct ConnectionLine : public SelectHoverObject, public ComponentListener
{
    using Ptr = ReferenceCountedObjectPtr<ConnectionLine>;

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;

    ConnectionLine(ConnectionPort& p1, ConnectionPort& p2);
    ~ConnectionLine(){
        inPort->setOtherPort(nullptr);
        outPort->setOtherPort(nullptr);
        inPort->removeComponentListener(this);
        outPort->removeComponentListener(this);
    }

    void paint(Graphics &g) override
    {
        int thiccness;
        if (hoverMode || selectMode) {
            g.setColour(Colours::blue);
            thiccness = 3;
        } else {
            g.setColour(Colours::black);
            thiccness = 2;
        }

        g.drawLine(line.toFloat(),thiccness);
    }

    bool hitTest(int x, int y) override;

    ConnectionPort::Ptr getOtherPort(const ConnectionPort::Ptr& port) {
        return inPort == port ? outPort : inPort;
    }

    ConnectionPort::Ptr getInPort() {
        return inPort;
    }

    ConnectionPort::Ptr getInPort() const {
        return inPort;
    }

    ConnectionPort::Ptr getOutPort() {
        return outPort;
    }

    ConnectionPort::Ptr getOutPort() const {
        return outPort;
    }

    void setInPort(ConnectionPort::Ptr newInPort) {
        inPort = newInPort;
    }

    void setOutPort(ConnectionPort::Ptr newOutPort) {
        outPort = newOutPort;
    }

    void setAudioConnection(AudioProcessorGraph::Connection connection) {
        audioConnection = connection;
    }

    AudioProcessorGraph::Connection getAudioConnection() const {
        return audioConnection;
    }

    struct IDs {
        static const Identifier CONNECTION_ID;
        static const Identifier InPort;
        static const Identifier OutPort;
        static const Identifier ConnectionLineObject;
        static const Identifier AudioConnection;
    };

private:
    Line<int> line;
    AudioProcessorGraph::Connection audioConnection;

    ConnectionPort::Ptr inPort;
    ConnectionPort::Ptr outPort;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectionLine)
};


struct LineComponent : public GuiObject
{
    LineComponent()
    {
        setBounds(0,0,getParentWidth(), getParentHeight());
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

    static LineComponent* getDragLine(){
        return dragLine;
    }

    static void setDragLine(LineComponent* newLine) {
        dragLine = newLine;
    }

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    ConnectionPort* port1 = nullptr;

private:
    static LineComponent* dragLine;

    Line<int> line;
    Point<int> p1, p2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LineComponent)
};

class GuiParameter : public GuiObject
{

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

