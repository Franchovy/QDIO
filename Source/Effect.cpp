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
LineComponent EffectTreeBase::dragLine;


const Identifier EffectTreeBase::IDs::effectTreeBase = "effectTreeBase";
const Identifier EffectTreeBase::IDs::pos = "pos";
const Identifier EffectTreeBase::IDs::processorID = "processor";
const Identifier EffectTreeBase::IDs::initialised = "initialised";
const Identifier EffectTreeBase::IDs::name = "name";


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

EffectTreeBase* EffectTreeBase::effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree) {
    // Check if event is leaving parent
    auto parent = getFromTree<EffectTreeBase>(effectTree);
    if (! parent->contains(parent->getLocalPoint(event.eventComponent, event.getPosition()))) {
        // find new parent
        std::cout << "Find new parent" << newLine;
        auto parentToCheck = parent->getParent();
        if (parentToCheck != nullptr
                && parentToCheck->contains(parentToCheck->getLocalPoint(event.eventComponent, event.getPosition())))
        {
            return parentToCheck;
        }
    } else {
        // Check if children match
        for (int i = 0; i < effectTree.getNumChildren(); i++) {
            auto childTree = effectTree.getChild(i);
            auto child = getFromTree<EffectTreeBase>(childTree);
            auto childEvent = event.getEventRelativeTo(child);

            if (child != nullptr
                && child->contains(childEvent.getPosition())
                && child != event.originalComponent) {
                // Add any filters here
                // Must be in edit mode
                if (!dynamic_cast<Effect *>(child)->isInEditMode()) { continue; }

                // Check if there's a match in the children (sending child component coordinates)
                if (auto e = effectToMoveTo(childEvent, childTree))
                    return e;
                else
                    return child;
            }
        }
    }
    return nullptr;
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
/*void EffectTreeBase::addEffect(const MouseEvent& event, const Effect& childEffect, bool addToMain) {
    auto parentTree = childEffect.getTree().getParent();
    parentTree.removeChild(childEffect.getTree(), nullptr);

    ValueTree newParent;
    if (auto newGUIEffect = dynamic_cast<Effect*>(event.eventComponent)){
        newParent = newGUIEffect->getTree();
    } else
        newParent = tree;

    if (newParent == tree) {
        std::cout << "Change me" << newLine;
    } else {
        std::cout << "Don't change me" << newLine;
    }


    newParent.appendChild(childEffect.getTree(), nullptr);
}*/

/*Effect* EffectTreeBase::createEffect(const AudioProcessorGraph::Node::Ptr& node)
{
    if (node != nullptr){
        // Individual effect from processor
        return new Effect(event, node->nodeID);
    }
    if (selected.getNumSelected() == 0){
        // Empty effect
        return new Effect(event);
    } else if (selected.getNumSelected() > 0){
        // Create Effect with selected Effects inside
        Array<const Effect*> effectVTArray;
        for (auto item : selected.getItemArray()){
            if (auto e = dynamic_cast<Effect*>(item.get()))
                effectVTArray.add(e);
        }
        selected.deselectAll();
        return new Effect(event, effectVTArray);
    }
}*/

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

PopupMenu EffectTreeBase::getEffectSelectMenu(const MouseEvent &event) {
    PopupMenu m;

    m.addItem("Empty Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("New Empty Effect");
                ValueTree newEffect(ID_EFFECT_VT);

                newEffect.setProperty(IDs::pos, Position::toVar(event.getPosition()), nullptr);
                this->getTree().appendChild(newEffect, &undoManager);

                if (selected.getNumSelected() > 0) {
                    for (auto s : selected.getItemArray()) {
                        if (auto e = dynamic_cast<Effect*>(s.get())){
                            newEffect.appendChild(e->getTree(), &undoManager);
                        }
                    }
                }
            }));
    m.addItem("Input Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("New Input Effect");
                ValueTree newEffect(ID_EFFECT_VT);
                newEffect.setProperty(IDs::pos, Position::toVar(event.getPosition()), nullptr);
                newEffect.setProperty(IDs::processorID, 0, nullptr);
                this->getTree().appendChild(newEffect, &undoManager);
            }));
    m.addItem("Output Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("New Output Effect");
                ValueTree newEffect(ID_EFFECT_VT);
                newEffect.setProperty(IDs::pos, Position::toVar(event.getPosition()), nullptr);
                newEffect.setProperty(IDs::processorID, 1, nullptr);
                this->getTree().appendChild(newEffect, &undoManager);
            }));
    m.addItem("Delay Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("New Delay Effect");
                ValueTree newEffect(ID_EFFECT_VT);
                newEffect.setProperty(IDs::pos, Position::toVar(event.getPosition()), nullptr);
                newEffect.setProperty(IDs::processorID, 3, nullptr);
                this->getTree().appendChild(newEffect, &undoManager);
            }
    ));
    m.addItem("Distortion Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("New Distortion Effect");
                ValueTree newEffect(ID_EFFECT_VT);
                newEffect.setProperty(IDs::pos, Position::toVar(event.getPosition()), nullptr);
                newEffect.setProperty(IDs::processorID, 2, nullptr);
                this->getTree().appendChild(newEffect, &undoManager);
            }
    ));

    return m;
}

void EffectTreeBase::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    // if effect has been created already
    if (childWhichHasBeenAdded.hasProperty(IDs::initialised)) {
        if (auto e = getFromTree<Effect>(childWhichHasBeenAdded)){
            e->setVisible(true);
            // Adjust pos
            if (auto parent = getFromTree<EffectTreeBase>(parentTree)) {
                if (!parent->getChildren().contains(e)) {
                    std::cout << "Add child to parent" << newLine;
                    e->setTopLeftPosition(parent->getLocalPoint(e, e->getPosition()));

                    parent->addAndMakeVisible(e);
                }
            }
        }
    } else {
        // Initialise VT
        childWhichHasBeenAdded.setProperty(IDs::initialised, true, &undoManager);
        // Create new effect
        new Effect(childWhichHasBeenAdded);
    }
}

template<class T>
T *EffectTreeBase::getFromTree(const ValueTree &vt) {
    return dynamic_cast<T*>(vt.getProperty(IDs::effectTreeBase).getObject());
}

void EffectTreeBase::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                           int indexFromWhichChildWasRemoved) {
    auto test = childWhichHasBeenRemoved.hasProperty(IDs::effectTreeBase);

    if (auto e = getFromTree<Effect>(childWhichHasBeenRemoved)) {
        if (auto parent = getFromTree<EffectTreeBase>(parentTree)) {
            std::cout << "Child removed" << newLine;

            // Adjust position
            //e->setTopLeftPosition(e->getPosition() + parent->getPosition());

            //TODO remove connections

            e->setVisible(false);
        }
    }
}

bool EffectTreeBase::keyPressed(const KeyPress &key) {

    if (key.getKeyCode() == 's') {
        std::cout << "Audiograph status: " << newLine;
        for (auto node : audioGraph->getNodes()) {
            if (node != nullptr) {
                std::cout << "node: " << node->nodeID.uid << newLine;
                std::cout << node->getProcessor()->getName() << newLine;
            } else {
                std::cout << "Null node." << newLine;
            }
        }
    }
    if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'z') {
        std::cout << "Undo: " << undoManager.getUndoDescription() << newLine;
        undoManager.undo();
    } else if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'Z') {
        std::cout << "Redo: " << undoManager.getRedoDescription() << newLine;
        undoManager.redo();
    }
}

EffectTreeBase::~EffectTreeBase() {
    tree.removeAllProperties(&undoManager);

    // Warning: may be a bad move.
    if (getReferenceCount() > 0) {
        std::cout << "Warning: Reference count greater than zero. Exceptions may occur" << newLine;
        resetReferenceCount();
    }
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




Effect::Effect(ValueTree& vt) : EffectTreeBase(vt) {
    tree = vt;

    if (vt.hasProperty(IDs::processorID)) {

        // Individual processor
        std::unique_ptr<AudioProcessor> newProcessor;
        int id = vt.getProperty(IDs::processorID);
        switch (id) {
            case 0:
                newProcessor = std::make_unique<InputDeviceEffect>();
                break;
            case 1:
                newProcessor = std::make_unique<OutputDeviceEffect>();
                break;
            case 2:
                newProcessor = std::make_unique<DistortionEffect>();
                break;
            case 3:
                newProcessor = std::make_unique<DelayEffect>();
                break;
            default:
                std::cout << "ProcessorID not found." << newLine;
        }

        node = audioGraph->addNode(move(newProcessor));

        // Create from node:
        setProcessor(node->getProcessor());
    } else {
        // Make edit mode true by default
        setEditMode(true);
    }

    setupTitle();
    setupMenu();

    Point<int> newPos = Position::fromVar(tree.getProperty(IDs::pos));
    setBounds(newPos.x, newPos.y, 200,200);

    addAndMakeVisible(resizer);

    // Set tree properties

    tree.addListener(this);

    // Name
    if (isIndividual()) {
        tree.setProperty(IDs::name, processor->getName(), nullptr);
    } else {
        tree.setProperty(IDs::name, "Effect", nullptr);
    }

    // Position
    pos.referTo(tree, IDs::pos, &undoManager);
    setPos(getPosition());

    // Set parent component
    auto parentTree = vt.getParent();
    if (parentTree.isValid()) {
        auto parent = getFromTree<EffectTreeBase>(parentTree);
        parent->addAndMakeVisible(this);
    }
}

void Effect::setupTitle() {
    Font titleFont(20, Font::FontStyleFlags::bold);
    title.setFont(titleFont);
    title.setBounds(30,30,200, title.getFont().getHeight());
    title.setColour(title.textColourId, Colours::black);
    title.setEditable(true);

    title.onTextChange = [=]{
        undoManager.beginNewTransaction("Name change to: " + title.getText(true));
        tree.setProperty(IDs::name, title.getText(true), &undoManager);
    };

    addAndMakeVisible(title);
}

void Effect::setupMenu() {
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
}

Effect::~Effect()
{
    // Delete processor from graph
    audioGraph->removeNode(node->nodeID);
}

// Processor hasEditor? What to do if processor is a predefined plugin
void Effect::setProcessor(AudioProcessor *processor) {
    setEditMode(false);
    this->processor = processor;

    // Processor settings (how best to do this?)
    node->getProcessor()->setPlayConfigDetails(
            processor->getTotalNumInputChannels(),
            processor->getTotalNumOutputChannels(),
            audioGraph->getSampleRate(),
            audioGraph->getBlockSize());

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

    // Setup parameters
    parameters = &processor->getParameterTree();
    setParameters(parameters);

    // Update
    resized();
    repaint();
}

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
    if (isIndividual())
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

        setBounds(getBounds().expanded(50));
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
    std::cout << "new transaction" << newLine;
    undoManager.beginNewTransaction(getName());

    if (event.mods.isLeftButtonDown()) {
        if (editMode) {
            //TODO make lasso functionality static
            /*if (event.mods.isLeftButtonDown() && event.originalComponent == this) {
                lasso.setVisible(true);
                lasso.beginLasso(event, this);
            }*/
        } else {
            // Drag
            setAlwaysOnTop(true);
            if (event.mods.isLeftButtonDown()) {
                dragger.startDraggingComponent(this, event);
            }
        }
    } else if (event.mods.isRightButtonDown()) {
        // Send info upwards for menu
        //TODO don't do this, call custom menu function
        getParentComponent()->mouseDown(event);
    }
}

void Effect::mouseDrag(const MouseEvent &event) {
    if (event.eventComponent == this) {
        dragger.dragComponent(this, event, &constrainer);

        // Manual constraint
        auto newX = jlimit<int>(0, getParentWidth() - getWidth(), getX());
        auto newY = jlimit<int>(0, getParentHeight() - getHeight(), getY());
/*

        if (newX != getX() || newY != getY())
            if (event.x<-(getWidth() / 2) || event.y<-(getHeight() / 2) ||
                                                     event.x>(getWidth() * 3 / 2) || event.y>(getHeight() * 3 / 2)) {
                auto newPos = dragDetachFromParentComponent();
                newX = newPos.x;
                newY = newPos.y;
            }
*/
        setTopLeftPosition(newX, newY);
    }

    /*if (lasso.isVisible())
        lasso.dragLasso(event);*/
    if (event.mods.isLeftButtonDown()) {
        // Effect drag
        if (auto newParent = effectToMoveTo(event, tree.getParent())) {
            // todo hoverOver undomanager
            hoverOver(newParent);
            tree.getParent().removeChild(tree, &undoManager);
            newParent->getTree().appendChild(tree, &undoManager);
            if (newParent != this) {
                SelectHoverObject::setHoverComponent(newParent);
            } else {
                SelectHoverObject::resetHoverObject();
            }
        }
    }
}

void Effect::mouseUp(const MouseEvent &event) {
    setAlwaysOnTop(false);

    // set (undoable) data
    setPos(getBoundsInParent().getPosition());

    // no reassignment
    if (event.eventComponent == event.originalComponent)
        return;


    /*if (lasso.isVisible())
        lasso.endLasso();*/

    if (event.mods.isLeftButtonDown()) {
        // If the component is an effect, respond to move effect event
        if (Effect *effect = dynamic_cast<Effect *>(event.originalComponent)) {
            if (event.getDistanceFromDragStart() < 10) {
                // Consider this a click and not a drag
                selected.addToSelection(dynamic_cast<GuiObject*>(event.eventComponent));
                event.eventComponent->repaint();
            }

            /*// Scan effect to apply move to
            auto newParent = effectToMoveTo(event.getEventRelativeTo(this), tree);

            if (auto e = dynamic_cast<Effect *>(newParent))
                if (!e->isInEditMode())
                    return;
            // target is not in edit mode
            if (effect->getParentComponent() != newParent) {
                setParent(*newParent);
                //addEffect(event.getEventRelativeTo(newParent), *effect->EVT);
            }*/
        }
        // If component is LineComponent, respond to line drag event
        else if (auto l = dynamic_cast<LineComponent *>(event.eventComponent)) {
            if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent)) {
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
}

void Effect::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    if (treeWhosePropertyHasChanged == tree) {
        if (property == IDs::pos && treeWhosePropertyHasChanged.hasProperty(property)) {
            auto property = treeWhosePropertyHasChanged.getProperty(IDs::pos).getArray();
            auto x = (int)(*property)[0];
            auto y = (int)(*property)[1];
            std::cout << "Property changed: " << newLine;
            std::cout << x << " " << y << newLine;

            auto xpos = (int)(*pos)[0];
            auto ypos = (int)(*pos)[1];
            std::cout << "Pos: " << newLine;
            std::cout << xpos << " " << ypos << newLine;

            if (x != 0 && y != 0) {
                std::cout << "Undo operation" << newLine;
                setTopLeftPosition(Point<int>(x,y));
            }
        } else if (property == IDs::name) {
            auto e = getFromTree<Effect>(treeWhosePropertyHasChanged);
            auto newName = treeWhosePropertyHasChanged.getProperty(IDs::name);

            e->setName(newName);
            e->title.setText(newName, sendNotificationAsync);
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
    pos = Position::toVar(newPos);
}

bool Effect::keyPressed(const KeyPress &key) {
    if (key == KeyPress::deleteKey) {
        // delete Effect
        tree.getParent().removeChild(tree, &undoManager);
        //delete this;
    }
    return EffectTreeBase::keyPressed(key);
}

void Effect::hoverOver(EffectTreeBase *newParent) {
    if (newParent->getWidth() < getParentWidth() || newParent->getHeight() < getParentHeight()) {
        setBounds(getBounds().expanded(-20));
    } else {
        setBounds(getBounds().expanded(20));
    }
}

Point<int> Position::fromVar(const var &v) {
    Array<var>* array = v.getArray();

    int x = (*array)[0];
    int y = (*array)[1];

    return Point<int>(x, y);
}

var Position::toVar(const Point<int> &t) {
    var x = t.getX();
    var y = t.getY();

    auto array = Array<var>();
    array.add(x);
    array.add(y);

    var v = array;

    return v;
}

