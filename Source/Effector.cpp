/*
  ==============================================================================

    Effector.cpp
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/

#include "Effector.h"

// Static members
std::unique_ptr<AudioProcessorGraph> EffectTreeBase::audioGraph = nullptr;
std::unique_ptr<AudioProcessorPlayer> EffectTreeBase::processorPlayer = nullptr;
std::unique_ptr<AudioDeviceManager> EffectTreeBase::deviceManager = nullptr;
UndoManager EffectTreeBase::undoManager;
ComponentSelection EffectTreeBase::selected;

const Identifier EffectVT::IDs::xPos = "xPos";
const Identifier EffectVT::IDs::yPos = "yPos";

LineComponent* LineComponent::dragLine = nullptr;



//==============================================================================
// Line Component methods

/**
 * @param event modified mouseEvent to use mainComponent coords.
 */
void LineComponent::mouseDown(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);

    if (port1 = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        p1 = getLocalPoint(port1, port1->centrePoint);
    }

    p2 = thisEvent.getPosition();

    line.setStart(p1);
    line.setEnd(p2);

    setVisible(true);
    repaint();
}

void LineComponent::mouseDrag(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);
    p2 = thisEvent.getPosition();

    line.setEnd(p2);
    repaint();

    // Pass hover detection to EffectScene
    getParentComponent()->mouseDrag(thisEvent);
}

void LineComponent::mouseUp(const MouseEvent &event) {
    setVisible(false);

    // Pass this event to EffectScene
    auto eventMain = event.getEventRelativeTo(this).withNewPosition(
            event.getPosition() - getPosition()
    );
    getParentComponent()->mouseUp(eventMain);
}

void LineComponent::convert(ConnectionPort *port2) {
    if (port1 != nullptr) {
        // Connect port1 to port2
        dynamic_cast<EffectTreeBase*>(getParentComponent())->createConnection(std::make_unique<ConnectionLine>(*port1, *port2));
    }
}

//==============================================================================
// GuiEffect methods

GuiEffect::GuiEffect (const MouseEvent &event, EffectVT* parentEVT) :
    EVT(parentEVT)
{
    addAndMakeVisible(resizer);

    //TODO assign the tree before creation of GuiEffect for this to work
    if (parentEVT->getTree().getParent().hasType(ID_EFFECT_VT)){
        auto sizeDef = dynamic_cast<GuiEffect*>(parentEVT->getTree().getParent()
                .getProperty(ID_EFFECT_GUI).getObject())->getWidth() / 3;
        setBounds(event.getPosition().x, event.getPosition().y, sizeDef, sizeDef);
    } else {
        setBounds(event.getPosition().x, event.getPosition().y, 200,200);
    }

    menu.addItem("Toggle Edit Mode", [=]() {
        toggleEditMode();
    });
    menu.addItem("Change Effect Image..", [=]() {
        FileChooser imgChooser ("Select Effect Image..",
                               File::getSpecialLocation (File::userHomeDirectory),
                               "*.jpg;*.png;*.gif");

        if (imgChooser.browseForFileToOpen())
        {
            image = ImageFileFormat::loadFrom(imgChooser.getResult());
        }
    });

    editMenu.addItem("Add Input Port", [=](){addPort(EVT->getDefaultBus(), true); resized(); });
    editMenu.addItem("Add Output Port", [=](){addPort(EVT->getDefaultBus(), false); resized(); });
    editMenu.addItem("Toggle Edit Mode", [=]() {
        toggleEditMode();
    });

    Font titleFont(20, Font::FontStyleFlags::bold);
    title.setFont(titleFont);
    title.setText("New Empty Effect", dontSendNotification);
    title.setBounds(30,30,200, title.getFont().getHeight());
    title.setColour(title.textColourId, Colours::black);
    title.setEditable(true);
    addAndMakeVisible(title);

    // Make edit mode by default
    setEditMode(true);
}

GuiEffect::~GuiEffect()
{

}

// Processor hasEditor? What to do if processor is a predefined plugin
void GuiEffect::setProcessor(AudioProcessor *processor) {
    setEditMode(false);
    individual = true;

    // Set up ports based on processor buses
    int numInputBuses = processor->getBusCount(true );
    int numBuses = numInputBuses + processor->getBusCount(false);
    for (int i = 0; i < numBuses; i++){
        bool isInput = (i < numInputBuses) ? true : false;
        // Get bus from processor
        auto bus = isInput ? processor->getBus(true, i) :
                processor->getBus(false, i - numInputBuses);
        // Check channel number - if 0 ignore
        if (bus->getNumberOfChannels() == 0)
            continue;
        // Create port - giving audiochannelset info and isInput bool
        addPort(bus, isInput);
    }
    title.setText(processor->getName(), dontSendNotification);

    // Setup parameters
    parameters = &processor->getParameterTree();
    setParameters(parameters);

    // Update
    resized();
    repaint();
}

void GuiEffect::paint (Graphics& g)
{
    // Draw outline rectangle
    g.setColour(Colours::black);
    Rectangle<float> outline(10,10,getWidth()-20, getHeight()-20);
    Path outlineRect;
    outlineRect.addRoundedRectangle(outline, 10, 3);
    PathStrokeType outlineStroke(1);

    // Hover rectangle
    g.setColour(Colours::blue);
    Path hoverRectangle;
    hoverRectangle.addRoundedRectangle(0, 0, getWidth(), getHeight(), 10, 10);
    PathStrokeType strokeType(3);

    if (hoverMode) {
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(hoverRectangle, hoverRectangle, thiccness, 2);
    } else if (selectMode)
        strokeType.createStrokedPath(hoverRectangle, hoverRectangle);

    if (selectMode || hoverMode)
        g.strokePath(hoverRectangle, strokeType);

    g.setColour(Colours::whitesmoke);
    if (!editMode) {
        g.setOpacity(1.f);
    } else {
        g.setOpacity(0.1f);
    }

    if (image.isNull()) {
        g.fillRoundedRectangle(outline, 10);
    } else {
        g.drawImage(image, outline);
    }

    g.setOpacity(1.f);

    if (editMode){
        float thiccness[] = {10, 10};
        outlineStroke.createDashedStroke(outlineRect, outlineRect, thiccness, 2);
    } else {
        outlineStroke.createStrokedPath(outlineRect, outlineRect);
        g.setColour(Colours::black);
    }
    g.strokePath(outlineRect, outlineStroke);
}

void GuiEffect::resized()
{
    // Position Ports
    inputPortPos = inputPortStartPos;
    outputPortPos = outputPortStartPos;
    for (auto p : inputPorts){
        p->setCentrePosition(portIncrement, inputPortPos);
        p->internalPort->setCentrePosition(portIncrement + 50, inputPortPos);
        inputPortPos += portIncrement;
    }
    for (auto p : outputPorts){
        p->setCentrePosition(getWidth() - portIncrement, outputPortPos);
        p->internalPort->setCentrePosition(getWidth() - portIncrement - 50, outputPortPos);
        outputPortPos += portIncrement;
    }

    title.setBounds(30,30,200, title.getFont().getHeight());
}

void GuiEffect::mouseDown(const MouseEvent &event) {
    setAlwaysOnTop(true);
    if (event.mods.isLeftButtonDown()) {
        dragData.previousParent = getParentComponent();
        dragData.previousPos = getPosition();
        dragger.startDraggingComponent(this, event);
        EVT->mouseDown(event);
    } else if (event.mods.isRightButtonDown())
        getParentComponent()->mouseDown(event);
}

void GuiEffect::mouseDrag(const MouseEvent &event) {
    if (event.eventComponent == this) {
        dragger.dragComponent(this, event, &constrainer);

        // Manual constraint
        auto newX = jlimit<int>(0, getParentWidth() - getWidth(), getX());
        auto newY = jlimit<int>(0, getParentHeight() - getHeight(), getY());

        if (newX != getX() || newY != getY())
            if (event.x<-(getWidth() / 2) || event.y<-(getHeight() / 2) ||
                                                     event.x>(getWidth() * 3 / 2) || event.y>(getHeight() * 3 / 2)) {
                auto newPos = dragDetachFromParentComponent();
                newX = newPos.x;
                newY = newPos.y;
            }

        setTopLeftPosition(newX, newY);
    }

    EVT->mouseDrag(event);

    getParentComponent()->mouseDrag(event);
}

void GuiEffect::mouseUp(const MouseEvent &event) {
    setAlwaysOnTop(false);

    EVT->pos.x = event.getPosition().x;
    EVT->pos.y = event.getPosition().y;
    //setPos(getPosition());

    EVT->mouseUp(event);

    getParentComponent()->mouseUp(event);
}

void GuiEffect::mouseEnter(const MouseEvent &event) {
    if (!dynamic_cast<GuiEffect*>(event.eventComponent)->hoverMode)
        hoverMode = true;
    repaint();

    getParentComponent()->mouseEnter(event);
    Component::mouseEnter(event);
}

void GuiEffect::mouseExit(const MouseEvent &event) {
    if (dynamic_cast<GuiEffect*>(event.eventComponent)->hoverMode)
        hoverMode = false;
    repaint();

    getParentComponent()->mouseExit(event);
    Component::mouseExit(event);
}

Point<int> GuiEffect::dragDetachFromParentComponent() {
    auto newPos = getPosition() + getParentComponent()->getPosition();
    auto parentParent = getParentComponent()->getParentComponent();
    getParentComponent()->removeChildComponent(this);
    parentParent->addAndMakeVisible(this);

    for (auto c : getChildren())
        std::cout << "Child position: " << c->getPosition().toString() << newLine;

    return newPos;
}

void GuiEffect::childrenChanged() {
    //TODO this function is probably useless
    std::cout << "Children changed, call move" << newLine;
    moved();
    Component::childrenChanged();
}

void GuiEffect::parentHierarchyChanged() {
    // Children of parents who receive the change signal should ignore it.
    if (currentParent == getParentComponent())
        return;

    if (getParentComponent() == nullptr){
        setTopLeftPosition(getPosition() + currentParent->getPosition());
        currentParent = nullptr;
    } else {
        if (hasBeenInitialised) {
            Component* parent = getParentComponent();
            while (parent != getTopLevelComponent()) {
                setTopLeftPosition(getPosition() - parent->getPosition());
                parent = parent->getParentComponent();
            }
        }
        currentParent = getParentComponent();
    }
    Component::parentHierarchyChanged();
}

/**
 * Get the port at the given location, if there is one
 * @param pos relative to this component (no conversion needed here)
 * @return nullptr if no match, ConnectionPort* if found
 */
ConnectionPort::Ptr GuiEffect::checkPort(Point<int> pos) {
    for (auto p : inputPorts) {
        if (p->contains(p->getLocalPoint(this, pos)))
            return p;
        else if (p->internalPort->contains(p->internalPort->getLocalPoint(this, pos)))
            return p->internalPort;
    }
    for (auto p : outputPorts) {
        if (p->contains(p->getLocalPoint(this, pos)))
            return p;
        else if (p->internalPort->contains(p->internalPort->getLocalPoint(this, pos)))
            return p->internalPort;
    }
    return nullptr;
}

void GuiEffect::setEditMode(bool isEditMode) {
    if (isIndividual() || editMode == isEditMode)
        return;
    // Turn on edit mode
    if (isEditMode) {
        for (auto c : getChildren()) {
            c->setAlwaysOnTop(true);
            if (!dynamic_cast<AudioPort*>(c))
                c->setInterceptsMouseClicks(true, true);

        }
        title.setMouseCursor(MouseCursor::IBeamCursor);
        title.setInterceptsMouseClicks(true, true);

        title.setColour(title.textColourId, Colours::whitesmoke);

    }
    // Turn off edit mode
    else if (!isEditMode) {
        for (auto c : getChildren()) {
            c->setAlwaysOnTop(false);
            if (!dynamic_cast<AudioPort*>(c))
                c->setInterceptsMouseClicks(false, false);
        }
        title.setMouseCursor(getMouseCursor());
        title.setInterceptsMouseClicks(false,false);
        title.setColour(title.textColourId, Colours::black);
    }

    editMode = isEditMode;
    repaint();
}

void GuiEffect::setParameters(const AudioProcessorParameterGroup *group) {
    // Individual
    for (auto param : group->getParameters(false)) {
        addParameter(param);
    }
    for (auto c : getChildren())
        std::cout << c->getName() << newLine;
}

void GuiEffect::addParameter(AudioProcessorParameter *param) {
    if (param->isBoolean()) {
        // add bool parameter
        ToggleButton* button = new ToggleButton();

        ButtonListener* listener = new ButtonListener(param);
        button->addListener(listener);
        button->setName("Button");
        button->setBounds(20,50, 100, 40);
    } else if (param->isDiscrete() && !param->getAllValueStrings().isEmpty()) {
        // add combo parameter
        ComboBox* comboBox = new ComboBox();

        for (auto s : param->getAllValueStrings())
            comboBox->addItem(s, param->getAllValueStrings().indexOf(s));

        ComboListener* listener = new ComboListener(param);

        comboBox->addListener(listener);
        comboBox->setName("ComboBox");
        comboBox->setBounds(20, 50, 100, 40);

        addAndMakeVisible(comboBox);
    } else if (param->isDiscrete()) {
        // add int parameter
        Slider* slider = new Slider();
        auto paramRange = dynamic_cast<RangedAudioParameter*>(param)->getNormalisableRange();

        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               1, paramRange.skew));
        SliderListener* listener = new SliderListener(param);
        slider->addListener(listener);
        slider->setName("Slider");
        slider->setBounds(20, 50, 100, 40);

        addAndMakeVisible(slider);
    } else {
        // add float parameter
        Slider* slider = new Slider();
        auto paramRange = dynamic_cast<RangedAudioParameter*>(param)->getNormalisableRange();

        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               paramRange.interval, paramRange.skew));
        SliderListener* listener = new SliderListener(param);
        slider->addListener(listener);
        slider->setName("Slider");
        slider->setBounds(20, 50, 200, 40);

        addAndMakeVisible(slider);
    }
}

void GuiEffect::moved() {
    Component::moved();
}

void GuiEffect::addPort(AudioProcessor::Bus *bus, bool isInput) {
    auto p = isInput ?
             inputPorts.add(std::make_unique<AudioPort>(isInput)) :
             outputPorts.add(std::make_unique<AudioPort>(isInput));
    p->bus = bus;
    addAndMakeVisible(p);

    resized();


    if (!individual) {
        addChildComponent(p->internalPort.get());
        Point<int> d;
        d = isInput ? Point<int>(50, 0) : Point<int>(-50, 0);

        p->internalPort->setCentrePosition(getLocalPoint(p, p->centrePoint + d));
        p->internalPort->setVisible(editMode);
    } else {
        p->internalPort->setVisible(false);
    }
}

//=================================================================================
// AudioPort methods

AudioPort::AudioPort(bool isInput) : ConnectionPort()
    , internalPort(new InternalConnectionPort(this, !isInput))
{

    hoverBox = Rectangle<int>(0,0,60,60);
    outline = Rectangle<int>(20, 20, 20, 20);
    centrePoint = Point<int>(30,30);
    setBounds(0,0,60, 60);

    this->isInput = isInput;
}

bool AudioPort::canConnect(ConnectionPort::Ptr& other) {
    if (this->isInput == other->isInput)
        return false;

    // Connect to AudioPort of mutual parent
    return (dynamic_cast<AudioPort *>(other.get())
            && other->getParent()->getParentComponent() == this->getParent()->getParentComponent())
    // Connect to ICP of containing parent effect
           || (dynamic_cast<InternalConnectionPort *>(other.get())
    && other->getParent() == this->getParent()->getParentComponent());
}

// ==============================================================================
// Resizer methods

void Resizer::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);

    getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                  startPos.y + event.getDistanceFromDragStartY());
    Component::mouseDrag(event);
}

//==============================================================================

/**
 * ENCAPSULATOR CONSTRUCTOR EffectVT of group of EffectVTs
 * @param effectVTSet
 */
EffectVT::EffectVT(const MouseEvent &event, Array<const EffectVT *> effectVTSet) :
        EffectVT(event)
{
    // Note top left and bottom right effects to have a size to set
    Point<int> topLeft;
    Point<int> bottomRight;
    auto thisBounds = guiEffect.getBoundsInParent();

    for (auto eVT : effectVTSet){
        // Set itself as parent of given children
        eVT->getTree().getParent().removeChild(eVT->getTree(), nullptr);
        tree.appendChild(eVT->getTree(), nullptr);

        // Update position
        auto bounds = eVT->getGUIEffect()->getBoundsInParent();
        thisBounds = thisBounds.getUnion(bounds);
    }

    thisBounds.expand(10,10);
    guiEffect.setBounds(thisBounds);
}

/**
 * INDIVIDUAL CONSTRUCTOR
 * Node - Individual GuiEffect / effectVT
 * @param nodeID
 */
EffectVT::EffectVT(const MouseEvent &event, AudioProcessorGraph::NodeID nodeID) :
        EffectVT(event)
{
    // Create from node:
    node = EffectVT::audioGraph->getNodeForId(nodeID);
    processor = node->getProcessor();
    tree.setProperty("Node", node.get(), nullptr);

    // Initialise with processor
    guiEffect.setProcessor(processor);
}

/**
 * Empty effectVT
 */
EffectVT::EffectVT(const MouseEvent &event) :
        EffectTreeBase(ID_EFFECT_VT),
        TreeData(&undoManager),
        guiEffect(event, this)
{
    // Setup tree properties
    tree.setProperty("Name", name, nullptr);
    tree.setProperty("Effect", this, nullptr);
    tree.setProperty("GUI", &guiEffect, nullptr);

    tree.addListener(this);

    pos.x.referTo(tree, IDs::xPos, &undoManager);
    pos.y.referTo(tree, IDs::yPos, &undoManager);

    setPos(guiEffect.getPosition());
}

EffectVT::~EffectVT()
{
    tree.removeAllProperties(nullptr);
    // Delete processor from graph
    EffectVT::audioGraph->removeNode(node->nodeID);
}

void EffectVT::addConnection(ConnectionLine *connection) {
    connections.add(connection);
    guiEffect.addAndMakeVisible(connection);
}

EffectVT::NodeAndPort EffectVT::getNode(ConnectionPort::Ptr &port) {
    NodeAndPort nodeAndPort;

    if (port->connectionLine == nullptr)
        return nodeAndPort;
    else if (isIndividual()) {
        nodeAndPort.node = node;
        nodeAndPort.port = dynamic_cast<AudioPort*>(port.get());
        nodeAndPort.isValid = true;
        return nodeAndPort;
    } else {
        ConnectionPort::Ptr portToCheck = nullptr;
        if (auto p = dynamic_cast<AudioPort*>(port.get())) {
            portToCheck = p->internalPort;
            portToCheck->incReferenceCount();
        }

        else if (auto p = dynamic_cast<InternalConnectionPort*>(port.get())) {
            portToCheck = p->audioPort;
            portToCheck->incReferenceCount();
        }

        if (portToCheck->connectionLine != nullptr) {
            auto otherPort = portToCheck->connectionLine->getOtherPort(portToCheck);
            nodeAndPort = otherPort->getParent()->EVT->getNode(
                    otherPort);
            return nodeAndPort;
        } else return nodeAndPort;
    }
}

AudioProcessorGraph::NodeID EffectVT::getNodeID() const {
    return node->nodeID;
}

void EffectVT::mouseDown(const MouseEvent &event) {
    std::cout << "Begin transaction at pos: " << pos.x << " " << pos.y << newLine;
    auto name = "poop " + String(pos.x) + " " + String(pos.y);
    undoManager.beginNewTransaction(name);

    if (event.mods.isLeftButtonDown() && event.originalComponent == this){
        lasso.setVisible(true);
        lasso.beginLasso(event, this);
    }
}
//TODO
void EffectVT::mouseDrag(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.dragLasso(event);
    if (event.mods.isLeftButtonDown()) {
        auto thisEvent = event.getEventRelativeTo(this);

        // Line drag
        /*if (dynamic_cast<LineComponent*>(event.eventComponent)) {
            // ParentToCheck is the container of possible things to connect to.
            Component* parentToCheck;
            if (dynamic_cast<AudioPort*>(event.originalComponent))
                parentToCheck = event.originalComponent->getParentComponent()->getParentComponent();
            else if (dynamic_cast<InternalConnectionPort*>(event.originalComponent))
                parentToCheck = event.originalComponent->getParentComponent();

            //auto connectPort = portToConnectTo(newEvent, parentToCheck)
            ConnectionPort::Ptr connectPort;
            // Get port to connect to (if there is one)
            auto newEvent = event.getEventRelativeTo(parentToCheck);

            ValueTree treeToCheck;
            if (parentToCheck != this)
                treeToCheck = dynamic_cast<GuiEffect*>(parentToCheck)->EVT->getTree();
            else
                treeToCheck = tree;
            connectPort = portToConnectTo(newEvent, treeToCheck);

            if (connectPort != nullptr) {
                setHoverComponent(connectPort);
            } else
                setHoverComponent(nullptr);

        }
            // Effect drag
        else*/
        if (auto effect = dynamic_cast<GuiEffect *>(event.eventComponent)){

            auto newParent = effectToMoveTo(thisEvent, tree);

            if (newParent != this)
                setHoverComponent(dynamic_cast<GuiObject*>(newParent));
            else
                setHoverComponent(nullptr);
        }
    }
}
//TODO
void EffectVT::mouseUp(const MouseEvent &event) {
    //TODO make GuiEffect and this one and the same

    // no reassignment
    if (event.eventComponent == event.originalComponent)
        return;

    if (auto newParent = dynamic_cast<GuiEffect*>(event.eventComponent)) {

    }

    if (lasso.isVisible())
        lasso.endLasso();

    if (event.mods.isLeftButtonDown()) {
        // If the component is an effect, respond to move effect event
        if (GuiEffect *effect = dynamic_cast<GuiEffect *>(event.originalComponent)) {
            if (event.getDistanceFromDragStart() < 10) {
                // Consider this a click and not a drag
                selected.addToSelection(dynamic_cast<GuiObject*>(event.eventComponent));
                event.eventComponent->repaint();
            }

            // Scan effect to apply move to
            auto newParent = effectToMoveTo(event.getEventRelativeTo(this), tree);

            if (auto e = dynamic_cast<GuiEffect *>(newParent))
                if (!e->isInEditMode())
                    return;
            // target is not in edit mode
            if (effect->getParentComponent() != newParent) {
                addEffect(event.getEventRelativeTo(newParent), *effect->EVT);
            }
        }
            // If component is LineComponent, respond to line drag event
        else if (auto l = dynamic_cast<LineComponent *>(event.eventComponent)) {
            if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent.get())) {
                l->convert(port);
            }
        }
    }

    for (auto i : selected){
        std::cout << i->getName() << newLine;
    }
}

void EffectVT::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    if (treeWhosePropertyHasChanged == tree) {
        if (property == IDs::xPos) {
            guiEffect.setTopLeftPosition(pos.x, guiEffect.getPosition().y);
            std::cout << "x: " << pos.x << newLine;
        } else if (property == IDs::yPos) {
            guiEffect.setTopLeftPosition(guiEffect.getPosition().x, pos.y);
            std::cout << "y: " << pos.y << newLine;
        }
    }
}

void EffectVT::valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) {
    if (treeWhoseParentHasChanged == tree) {

    }
}

//==============================================================================
// Lasso

void EffectTreeBase::findLassoItemsInArea(Array<GuiObject::Ptr> &results, const Rectangle<int> &area) {
    for (auto c : componentsToSelect) {
        if (!intersectMode) {
            if (area.contains(c->getBoundsInParent())) {
                results.addIfNotAlreadyThere(c);
            }
        } else {
            if (area.intersects(c->getBoundsInParent())) {
                results.addIfNotAlreadyThere(c);
            }
        }
    }
}

SelectedItemSet<GuiObject::Ptr>& EffectTreeBase::getLassoSelection() {
    selected.clear();
    return selected;
}
//TODO
void EffectTreeBase::setHoverComponent(GuiObject::Ptr c) {
    if (auto e = dynamic_cast<GuiEffect*>(hoverComponent.get())) {
        e->hoverMode = false;
        e->repaint();
    } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent.get())) {
        p->hoverMode = false;
        p->repaint();
    }

    hoverComponent = c;

    if (auto e = dynamic_cast<GuiEffect*>(hoverComponent.get())) {
        e->hoverMode = true;
        e->repaint();
    } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent.get())) {
        p->hoverMode = true;
        p->repaint();
    }
}
//TODO
Component* EffectTreeBase::effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree) {
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e_gui = dynamic_cast<GuiEffect*>(effectTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());

        if (e_gui != nullptr
            && e_gui->getBoundsInParent().contains(event.x, event.y)
            && e_gui != event.originalComponent)
        {
            // Add any filters here
            if (e_gui->isIndividual()){
                continue;
            }

            // Check if there's a match in the children (sending child component coordinates)
            auto recursiveEvent = event.getEventRelativeTo(e_gui);
            if (auto e = effectToMoveTo(recursiveEvent, effectTree.getChild(i)))
                return e;
            else
                return e_gui;
        }
    }
    // If nothing is found (at topmost level) then return the maincomponent
    if (effectTree.hasType(ID_TREE_TOP))
        return this;
    else return nullptr;
}
//TODO
ConnectionPort::Ptr EffectTreeBase::portToConnectTo(MouseEvent& event, const ValueTree& effectTree) {

    // Check for self ports
    if (auto p = dynamic_cast<ConnectionPort*>(event.originalComponent))
        if (auto e = dynamic_cast<GuiEffect*>(event.eventComponent))
            if (auto returnPort = e->checkPort(event.getPosition()))
                if (p->canConnect(returnPort))
                    return returnPort;

    // Check children for a match
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e_gui = dynamic_cast<GuiEffect*>(effectTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());

        if (e_gui == nullptr)
            continue;

        // Filter self effect if AudioPort
        if (auto p = dynamic_cast<AudioPort*>(event.originalComponent))
            if (p->getParent() == e_gui)
                continue;

        auto localPos = e_gui->getLocalPoint(event.eventComponent, event.getPosition());

        std::cout << "Local event: " <<  e_gui->getLocalPoint(event.eventComponent, event.getPosition()).toString() << newLine;
        std::cout << "event pos: " << event.getPosition().toString() << newLine;

        if (e_gui->contains(localPos))
        {
            if (auto p = e_gui->checkPort(localPos))
                if (dynamic_cast<ConnectionPort*>(event.originalComponent)->canConnect(p))
                    return p;
        }
    }

    // If nothing is found return nullptr
    return nullptr;
}

/**
 * Adds existing effect as child to the effect under the mouse
 * @param event for which the location will determine what effect to add to.
 * @param childEffect effect to add
 */
//TODO
void EffectTreeBase::addEffect(const MouseEvent& event, const EffectVT& childEffect, bool addToMain) {
    auto parentTree = childEffect.getTree().getParent();
    parentTree.removeChild(childEffect.getTree(), nullptr);

    ValueTree newParent;
    if (auto newGUIEffect = dynamic_cast<GuiEffect*>(event.eventComponent)){
        newParent = newGUIEffect->EVT->getTree();
    } else
        newParent = tree;

    newParent.appendChild(childEffect.getTree(), nullptr);
}

//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GuiEffect" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

void ConnectionLine::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    line.setStart(getLocalPoint(inPort.get(), inPort->centrePoint));
    line.setEnd(getLocalPoint(outPort.get(), outPort->centrePoint));

    repaint();
}

bool ConnectionLine::hitTest(int x, int y) {
    auto d1 = line.getStart().getDistanceFrom(Point<int>(x,y));
    auto d2 = line.getEnd().getDistanceFrom(Point<int>(x,y));
    auto d = d1 + d2 - line.getLength();

    if (d < 7) {
        return true;
    } else {
        return false;
    }
}

ConnectionLine::ConnectionLine(ConnectionPort &p1, ConnectionPort &p2) {
    // Remember that input ports are the line output and vice versa
    if (!p1.isInput) {
        inPort = &p1;
        outPort = &p2;
    } else {
        inPort = &p2;
        outPort = &p1;
    }

    line = Line<int>(inPort->getParentComponent()->getPosition() + inPort->getPosition() + inPort->centrePoint,
                     outPort->getParentComponent()->getPosition() + outPort->getPosition() + outPort->centrePoint);

    inPort->getParent()->addComponentListener(this);
    outPort->getParent()->addComponentListener(this);

    inPort->connectionLine = this;
    outPort->connectionLine = this;

    setBounds(0,0,getParentWidth(),getParentHeight());
}

void ConnectionPort::mouseDown(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDown(event);
}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDrag(event);
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseUp(event);
}

void ConnectionPort::paint(Graphics &g) {
    g.setColour(Colours::black);
    //rectangle.setPosition(10,10);
    g.drawRect(outline,2);

    // Hover rectangle
    g.setColour(Colours::blue);
    Path drawPath;
    drawPath.addRoundedRectangle(hoverBox, 10, 10);
    PathStrokeType strokeType(3);

    if (hoverMode) {
        float thiccness[] = {2, 3};
        strokeType.createDashedStroke(drawPath, drawPath, thiccness, 2);
        g.strokePath(drawPath, strokeType);
    }
}

GuiEffect *ConnectionPort::getParent() {
    return dynamic_cast<GuiEffect*>(getParentComponent());
}

bool InternalConnectionPort::canConnect(ConnectionPort::Ptr& other) {
    // Return false if the port is AP and belongs to the same parent
    return !(dynamic_cast<AudioPort *>(other.get())
             && this->getParent() == other->getParent());
}


void EffectTreeBase::createConnection(std::unique_ptr<ConnectionLine> line) {
    // Add connection to this object
    connections.add(move(line));

    auto outputPort = line->inPort;
    auto inputPort = line->outPort;

    // Remember that an inputPort is receiving, on the output effect
    // and the outputPort is source on the input effect
    auto output = dynamic_cast<GuiEffect*>(outputPort->getParent());
    auto input = dynamic_cast<GuiEffect*>(inputPort->getParent());


    //TODO Must replace all this with in-subclass check and set methods!

    // Check for common parent
    // Find common parent
    if (input->getParentComponent() == output->getParentComponent()) {
        std::cout << "Common parent" << newLine;
        if (input->getParentComponent() == this) {
            addAndMakeVisible(line.get());
        } else {
            dynamic_cast<GuiEffect*>(input->getParentComponent())->EVT->addConnection(line.get());
        }
    } else if (input->getParentComponent() == output) {
        std::cout << "Output parent" << newLine;
        if (output->getParentComponent() == this) {
            addAndMakeVisible(line.get());
        } else {
            dynamic_cast<GuiEffect*>(output->getParentComponent())->EVT->addConnection(line.get());
        }
    } else if (output->getParentComponent() == input) {
        std::cout << "Input parent" << newLine;
        if (input->getParentComponent() == this) {
            addAndMakeVisible(line.get());
        } else {
            dynamic_cast<GuiEffect*>(input->getParentComponent())->EVT->addConnection(line.get());
        }
    }

    // Update audiograph
    addAudioConnection(*line);
}

void EffectTreeBase::addAudioConnection(ConnectionLine& connectionLine) {
    EffectVT::NodeAndPort in;
    EffectVT::NodeAndPort out;
    auto inEVT = connectionLine.inPort->getParent()->EVT;
    auto outEVT = connectionLine.outPort->getParent()->EVT;

    in = inEVT->getNode(connectionLine.inPort);
    out = outEVT->getNode(connectionLine.outPort);

    if (in.isValid && out.isValid) {
        for (int c = 0; c < jmin(EffectTreeBase::audioGraph->getTotalNumInputChannels(), EffectTreeBase::audioGraph->getTotalNumOutputChannels()); c++) {
            AudioProcessorGraph::Connection connection = {{in.node->nodeID, in.port->bus->getChannelIndexInProcessBlockBuffer(c)},
                                                          {out.node->nodeID, out.port->bus->getChannelIndexInProcessBlockBuffer(c)}};
            if (!EffectTreeBase::audioGraph->isConnected(connection) && EffectTreeBase::audioGraph->isConnectionLegal(connection)) {
                EffectTreeBase::audioGraph->addConnection(connection);
            }
        }
    }
}


void EffectTreeBase::initialiseAudio(std::unique_ptr<AudioProcessorGraph> graph, std::unique_ptr<AudioDeviceManager> dm,
                                     std::unique_ptr<AudioProcessorPlayer> pp, std::unique_ptr<XmlElement> savedState)
{
    audioGraph = move(graph);
    audioGraph->enableAllBuses();

    EffectTreeBase::deviceManager = move(dm);
    deviceManager->initialise (256, 256, savedState.get(), true);

    EffectTreeBase::processorPlayer = move(pp);
    deviceManager->addAudioCallback (processorPlayer.get());
    processorPlayer->setProcessor (audioGraph.get());

}

void EffectTreeBase::close() {
    processorPlayer->setProcessor(nullptr);
    deviceManager->closeAudioDevice();

    deviceManager.release();
    processorPlayer.release();
    audioGraph.release();
}


