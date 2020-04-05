/*
  ==============================================================================

    Effect.cpp
    Created: 31 Mar 2020 9:03:16pm
    Author:  maxime

  ==============================================================================
*/

#include "Effect.h"

// Static members
std::unique_ptr<AudioProcessorGraph> EffectTreeBase::audioGraph = nullptr;
std::unique_ptr<AudioProcessorPlayer> EffectTreeBase::processorPlayer = nullptr;
std::unique_ptr<AudioDeviceManager> EffectTreeBase::deviceManager = nullptr;
UndoManager EffectTreeBase::undoManager;
ComponentSelection EffectTreeBase::selected;
GuiObject::Ptr EffectTreeBase::hoverComponent = nullptr;


const Identifier EffectTreeBase::IDs::effectTreeBase = "effectTreeBase";

const Identifier Effect::IDs::xPos = "xPos";
const Identifier Effect::IDs::yPos = "yPos";

// Processor hasEditor? What to do if processor is a predefined plugin
void Effect::setProcessor(AudioProcessor *processor) {
    setEditMode(false);

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

Point<int> EffectTreeBase::dragDetachFromParentComponent() {
    auto newPos = getPosition() + getParentComponent()->getPosition();
    auto parentParent = getParentComponent()->getParentComponent();
    getParentComponent()->removeChildComponent(this);
    parentParent->addAndMakeVisible(this);

    for (auto c : getChildren())
        std::cout << "Child position: " << c->getPosition().toString() << newLine;

    return newPos;
}


void EffectTreeBase::createConnection(std::unique_ptr<ConnectionLine> line) {
    // Add connection to this object
    addAndMakeVisible(line.get());
    auto nline = connections.add(move(line));

    // Update audiograph
    addAudioConnection(*nline);
}



/*void GuiEffect::parentHierarchyChanged() {
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
}*/

/**
 * Get the port at the given location, if there is one
 * @param pos relative to this component (no conversion needed here)
 * @return nullptr if no match, ConnectionPort* if found
 */
ConnectionPort::Ptr Effect::checkPort(Point<int> pos) {
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

void Effect::setEditMode(bool isEditMode) {
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

void Effect::setParameters(const AudioProcessorParameterGroup *group) {
    // Individual
    for (auto param : group->getParameters(false)) {
        addParameter(param);
    }
    for (auto c : getChildren())
        std::cout << c->getName() << newLine;
}

void Effect::addParameter(AudioProcessorParameter *param) {
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


void Effect::addPort(AudioProcessor::Bus *bus, bool isInput) {
    auto p = isInput ?
             inputPorts.add(std::make_unique<AudioPort>(isInput)) :
             outputPorts.add(std::make_unique<AudioPort>(isInput));
    p->bus = bus;
    addAndMakeVisible(p);

    resized();

    if (!isIndividual()) {
        addChildComponent(p->internalPort.get());
        Point<int> d;
        d = isInput ? Point<int>(50, 0) : Point<int>(-50, 0);

        p->internalPort->setCentrePosition(getLocalPoint(p, p->centrePoint + d));
        p->internalPort->setVisible(editMode);
    } else {
        p->internalPort->setVisible(false);
    }
}


/**
 * ENCAPSULATOR CONSTRUCTOR Effect of group of EffectVTs
 * @param effectVTSet
 */
Effect::Effect(const MouseEvent &event, Array<const Effect *> effectVTSet) :
        Effect(event)
{
    // Note top left and bottom right effects to have a size to set
    Point<int> topLeft;
    Point<int> bottomRight;
    auto thisBounds = getBoundsInParent();

    for (auto eVT : effectVTSet){
        // Set itself as parent of given children
        tree.getParent().removeChild(tree, nullptr);
        tree.appendChild(eVT->getTree(), nullptr);

        // Update position
        auto bounds = eVT->getBoundsInParent();
        thisBounds = thisBounds.getUnion(bounds);
    }

    thisBounds.expand(10,10);
    setBounds(thisBounds);
}

/**
 * INDIVIDUAL CONSTRUCTOR
 * Node - Individual GuiEffect / effectVT
 * @param nodeID
 */
Effect::Effect(const MouseEvent &event, AudioProcessorGraph::NodeID nodeID) :
        Effect(event)
{
    // Create from node:
    node = Effect::audioGraph->getNodeForId(nodeID);
    processor = node->getProcessor();
    tree.setProperty("Node", node.get(), nullptr);

    // Initialise with processor
    setProcessor(processor);
}

/**
 * Empty effectVT
 */
Effect::Effect(const MouseEvent &event) :
        EffectTreeBase(ID_EFFECT_VT)
{
    setBounds(event.getPosition().x, event.getPosition().y, 200,200);

    menu.addItem("Toggle Edit Mode", [=]() {
        setEditMode(!editMode);
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

    editMenu.addItem("Add Input Port", [=](){addPort(getDefaultBus(), true); resized(); });
    editMenu.addItem("Add Output Port", [=](){addPort(getDefaultBus(), false); resized(); });
    editMenu.addItem("Toggle Edit Mode", [=]() {
        setEditMode(!editMode);
    });

    Font titleFont(20, Font::FontStyleFlags::bold);
    title.setFont(titleFont);
    title.setText("New Empty Effect", dontSendNotification);
    title.setBounds(30,30,200, title.getFont().getHeight());
    title.setColour(title.textColourId, Colours::black);
    title.setEditable(true);
    addAndMakeVisible(title);

    addAndMakeVisible(resizer);

    // Setup tree properties
    tree.setProperty("Name", name, nullptr);
    tree.setProperty("Effect", this, nullptr);

    tree.addListener(this);

    pos.x.referTo(tree, IDs::xPos, &undoManager);
    pos.y.referTo(tree, IDs::yPos, &undoManager);

    setPos(getPosition());

    // Make edit mode by default
    setEditMode(true);
}

Effect::~Effect()
{
    tree.removeAllProperties(nullptr);
    // Delete processor from graph
    Effect::audioGraph->removeNode(node->nodeID);
}


Effect::NodeAndPort Effect::getNode(ConnectionPort::Ptr &port) {
    NodeAndPort nodeAndPort;

    if (!port->isConnected())
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

        if (portToCheck->isConnected()) {
            auto otherPort = portToCheck->getOtherPort();
            nodeAndPort = dynamic_cast<Effect*>(otherPort->getParentComponent())->getNode(
                    otherPort);
            return nodeAndPort;
        } else return nodeAndPort;
    }
}

AudioProcessorGraph::NodeID Effect::getNodeID() const {
    return node->nodeID;
}

void Effect::mouseDown(const MouseEvent &event) {
    auto name = "poop " + String(pos.x) + " " + String(pos.y);
    undoManager.beginNewTransaction(name);

    if (event.mods.isLeftButtonDown()) {
        if (editMode) {
            // Lasso
            if (event.mods.isLeftButtonDown() && event.originalComponent == this) {
                lasso.setVisible(true);
                lasso.beginLasso(event, this);
            }
        } else {
            // Drag
            setAlwaysOnTop(true);
            if (event.mods.isLeftButtonDown()) {
                dragger.startDraggingComponent(this, event);
            }
        }
    } else if (event.mods.isRightButtonDown()) {
        // Send info upwards for menu
        getParentComponent()->mouseDown(event);
    }
}

void Effect::mouseDrag(const MouseEvent &event) {
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

    getParentComponent()->mouseDrag(event);

    if (lasso.isVisible())
        lasso.dragLasso(event);
    if (event.mods.isLeftButtonDown()) {
        auto thisEvent = event.getEventRelativeTo(this);

        // Effect drag
        if (dynamic_cast<Effect *>(event.eventComponent)){

            auto newParent = effectToMoveTo(thisEvent, tree);

            if (newParent != this)
                setHoverComponent(dynamic_cast<GuiObject*>(newParent));
            else
                setHoverComponent(nullptr);
        }
    }
}

void Effect::mouseUp(const MouseEvent &event) {
    setAlwaysOnTop(false);

    setPos(getPosition());
    // no reassignment
    if (event.eventComponent == event.originalComponent)
        return;

    if (auto newParent = dynamic_cast<EffectTreeBase*>(event.eventComponent)) {

    }

    if (lasso.isVisible())
        lasso.endLasso();

    if (event.mods.isLeftButtonDown()) {
        // If the component is an effect, respond to move effect event
        if (Effect *effect = dynamic_cast<Effect *>(event.originalComponent)) {
            if (event.getDistanceFromDragStart() < 10) {
                // Consider this a click and not a drag
                selected.addToSelection(dynamic_cast<GuiObject*>(event.eventComponent));
                event.eventComponent->repaint();
            }

            // Scan effect to apply move to
            auto newParent = effectToMoveTo(event.getEventRelativeTo(this), tree);

            if (auto e = dynamic_cast<Effect *>(newParent))
                if (!e->isInEditMode())
                    return;
            // target is not in edit mode
            if (effect->getParentComponent() != newParent) {
                setParent(*newParent);
                //addEffect(event.getEventRelativeTo(newParent), *effect->EVT);
            }
        }
            // If component is LineComponent, respond to line drag event
        else if (auto l = dynamic_cast<LineComponent *>(event.eventComponent)) {
            if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent.get())) {
                if (l->port1 != nullptr) {
                    // todo this must be called on common parent.
                    dynamic_cast<EffectTreeBase*>(getParentComponent())
                         ->createConnection(std::make_unique<ConnectionLine>(*l->port1, *port));
                }
            }
        }
    }

    for (auto i : selected){
        std::cout << i->getName() << newLine;
    }

    getParentComponent()->mouseUp(event);
}

void Effect::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    if (treeWhosePropertyHasChanged == tree) {
        if (property == IDs::xPos) {
            setTopLeftPosition(pos.x, getPosition().y);
            std::cout << "x: " << pos.x << newLine;
        } else if (property == IDs::yPos) {
            setTopLeftPosition(getPosition().x, pos.y);
            std::cout << "y: " << pos.y << newLine;
        }
    }
}

void Effect::valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) {
    if (treeWhoseParentHasChanged == tree) {

    }
}

void Effect::resized() {
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

void Effect::mouseEnter(const MouseEvent &event) {
    if (hoverMode)
        hoverMode = true;
    repaint();

    getParentComponent()->mouseEnter(event);
    Component::mouseEnter(event);
}

void Effect::mouseExit(const MouseEvent &event) {
    if (hoverMode)
        hoverMode = false;
    repaint();

    getParentComponent()->mouseExit(event);
    Component::mouseExit(event);
}

void Effect::paint(Graphics& g) {
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

void Effect::setParent(EffectTreeBase &parent) {
    parent.getTree().appendChild(tree, &undoManager);
}

void Effect::setPos(Point<int> newPos) {
    pos.x = newPos.x;
    pos.y = newPos.y;
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
    if (auto e = dynamic_cast<Effect*>(hoverComponent.get())) {
        e->setHoverMode(false);
        e->repaint();
    } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent.get())) {
        p->hoverMode = false;
        p->repaint();
    }

    hoverComponent = c;

    if (auto e = dynamic_cast<Effect*>(hoverComponent.get())) {
        e->setHoverMode(true);
        e->repaint();
    } else if (auto p = dynamic_cast<ConnectionPort*>(hoverComponent.get())) {
        p->hoverMode = true;
        p->repaint();
    }
}
//TODO
EffectTreeBase* EffectTreeBase::effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree) {
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e_gui = dynamic_cast<Effect*>(effectTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());

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
        if (auto e = dynamic_cast<Effect*>(event.eventComponent))
            if (auto returnPort = e->checkPort(event.getPosition()))
                if (p->canConnect(returnPort))
                    return returnPort;

    // Check children for a match
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e_gui = dynamic_cast<Effect*>(effectTree.getChild(i).getProperty(ID_EFFECT_GUI).getObject());

        if (e_gui == nullptr)
            continue;

        // Filter self effect if AudioPort
        if (auto p = dynamic_cast<AudioPort*>(event.originalComponent))
            if (p->getParentComponent() == e_gui)
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
void EffectTreeBase::addEffect(const MouseEvent& event, const Effect& childEffect, bool addToMain) {
    auto parentTree = childEffect.getTree().getParent();
    parentTree.removeChild(childEffect.getTree(), nullptr);

    ValueTree newParent;
    if (auto newGUIEffect = dynamic_cast<Effect*>(event.eventComponent)){
        newParent = newGUIEffect->getTree();
    } else
        newParent = tree;


    newParent.appendChild(childEffect.getTree(), nullptr);
}



void EffectTreeBase::addAudioConnection(ConnectionLine& connectionLine) {
    Effect::NodeAndPort in;
    Effect::NodeAndPort out;
    auto inEVT = dynamic_cast<Effect*>(connectionLine.inPort->getParentComponent());
    auto outEVT = dynamic_cast<Effect*>(connectionLine.outPort->getParentComponent());

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


