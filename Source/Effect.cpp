/*
  ==============================================================================

    Effect.cpp
    Created: 31 Mar 2020 9:03:16pm
    Author:  maxime

  ==============================================================================
*/

#define DEBUG_UTILITIES

#include "Effect.h"
#include "IDs"
#include "DSPEffects.h"

// Static members
EffectTreeBase::AppState EffectTreeBase::appState = neutral;
AudioProcessorGraph* EffectTreeBase::audioGraph = nullptr;
AudioProcessorPlayer* EffectTreeBase::processorPlayer = nullptr;
AudioDeviceManager* EffectTreeBase::deviceManager = nullptr;
UndoManager EffectTreeBase::undoManager;
LineComponent EffectTreeBase::dragLine;


const Identifier EffectTreeBase::IDs::effectTreeBase = "effectTreeBase";

//const Identifier Effect::IDs::pos = "pos";
const Identifier Effect::IDs::x = "x";
const Identifier Effect::IDs::y = "y";
const Identifier Effect::IDs::w = "w";
const Identifier Effect::IDs::h = "h";
const Identifier Effect::IDs::processorID = "processor";
const Identifier Effect::IDs::initialised = "initialised";
const Identifier Effect::IDs::name = "name";
const Identifier Effect::IDs::connections = "connections";
const Identifier Effect::IDs::EFFECT_ID = "effect";


void EffectTreeBase::findLassoItemsInArea(Array<SelectHoverObject::Ptr> &results, const Rectangle<int> &area) {
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

SelectedItemSet<SelectHoverObject::Ptr>& EffectTreeBase::getLassoSelection() {
    selected.clear();
    return selected;
}

EffectTreeBase* EffectTreeBase::effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree) {
    /*// Check if event is leaving parent
    auto parent = getFromTree<EffectTreeBase>(effectTree);
    if (! parent->contains(parent->getLocalPoint(event.eventComponent, event.getPosition()))) {
        // find new parent
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
            if (childTree.hasType(EFFECT_ID)) {
                auto child = getFromTree<Effect>(childTree);

                if (child != nullptr) {
                    auto childEvent = event.getEventRelativeTo(child);
                    if (child->contains(childEvent.getPosition())
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
        }
    }*/
    return nullptr;
}

//TODO
ConnectionPort* EffectTreeBase::portToConnectTo(const MouseEvent& event, const ValueTree& effectTree) {
/*
    ValueTree effectTreeToCheck;
    if (dynamic_cast<AudioPort*>(event.originalComponent) || dynamic_cast<ParameterPort*>(event.originalComponent))
    {
        effectTreeToCheck = effectTree.getParent();
    }
    else
    {
        effectTreeToCheck = effectTree;
    }

    // Check for self ports
    if (auto p = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        if (auto e = getFromTree<Effect>(effectTreeToCheck)) {
            if (auto returnPort = e->checkPort(e->getLocalPoint(event.eventComponent, event.getPosition()))) {
                if (p->canConnect(returnPort)) {
                    return returnPort;
                }
            }
        }
    }

    // Check children for a match
    for (int i = 0; i < effectTreeToCheck.getNumChildren(); i++) {

        if (effectTreeToCheck.getChild(i).hasType(EFFECT_ID)) {
            auto e = getFromTree<Effect>(effectTreeToCheck.getChild(i));

            if (e == nullptr)
                continue;

            // Filter self effect if AudioPort
            if (auto p = dynamic_cast<AudioPort *>(event.originalComponent))
                if (p->getParentComponent() == e)
                    continue;

            auto localPos = e->getLocalPoint(event.originalComponent, event.getPosition());

            if (e->contains(localPos)) {
                if (auto p = e->checkPort(localPos))
                    if (dynamic_cast<ConnectionPort *>(event.originalComponent)->canConnect(p))
                        return p;
            }
        } else if (effectTreeToCheck.getChild(i).hasType(PARAMETER_ID)) {
            jassert(effectTreeToCheck.getChild(i).hasProperty(Parameter::IDs::parameterObject));

            auto parameter = getFromTree<Parameter>(effectTreeToCheck.getChild(i));

            bool isInternal = dynamic_cast<ConnectionPort *>(event.originalComponent)->isInput;
            auto parameterPortToCheck = parameter->getPort(! isInternal);

            auto localPos = parameter->getPort(! isInternal)->getLocalPoint(event.originalComponent, event.getPosition());
            if (parameterPortToCheck->contains(localPos)) {
                if (dynamic_cast<ConnectionPort *>(event.originalComponent)->canConnect(parameterPortToCheck)) {
                    return parameterPortToCheck;
                }
            }
        }
    }*/

    // If nothing is found return nullptr
    return nullptr;
}

bool EffectTreeBase::connectAudio(const ConnectionLine& connectionLine) {
    for (auto connection : getAudioConnection(connectionLine)) {
        if (!EffectTreeBase::audioGraph->isConnected(connection) &&
            EffectTreeBase::audioGraph->isConnectionLegal(connection)) {
            // Make audio connection
            return EffectTreeBase::audioGraph->addConnection(connection);
        }
    }
}

void EffectTreeBase::disconnectAudio(const ConnectionLine &connectionLine) {
    for (auto connection : getAudioConnection(connectionLine)) {
        if (audioGraph->isConnected(connection)) {
            audioGraph->removeConnection(connection);
        }
    }
}

Array<AudioProcessorGraph::Connection> EffectTreeBase::getAudioConnection(const ConnectionLine& connectionLine) {
    Effect::NodeAndPort in;
    Effect::NodeAndPort out;

    auto inPort = connectionLine.getInPort();
    auto outPort = connectionLine.getOutPort();

    auto inEVT = dynamic_cast<Effect *>(inPort->getParentComponent());
    auto outEVT = dynamic_cast<Effect *>(outPort->getParentComponent());

    in = inEVT->getNode(inPort);
    out = outEVT->getNode(outPort);

    auto returnArray = Array<AudioProcessorGraph::Connection>();

    if (in.isValid && out.isValid) {
        for (int c = 0; c < jmin(EffectTreeBase::audioGraph->getTotalNumInputChannels(),
                                 EffectTreeBase::audioGraph->getTotalNumOutputChannels()); c++) {
            AudioProcessorGraph::Connection connection = {{in.node->nodeID,  in.port->bus->getChannelIndexInProcessBlockBuffer(
                    c)},
                                                          {out.node->nodeID, out.port->bus->getChannelIndexInProcessBlockBuffer(
                                                                  c)}};
            returnArray.add(connection);
        }
    }
    return returnArray;
}


void EffectTreeBase::close() {
    SelectHoverObject::close();

}


/*
void EffectTreeBase::createGroupEffect() {
    undoManager.beginNewTransaction("Create Group Effect");
    // Create new effect
    auto newEffectTree = newEffect("Effect", -1);
    auto newE = getFromTree<Effect>(newEffectTree);
    newE->setSize(300, 300);

    // Save any connections to be broken
    Array<ConnectionLine::Ptr> connectionsToChange;

    for (int i = 0; i < tree.getNumChildren(); i++) {
        auto child = tree.getChild(i);
        if (child.hasType(CONNECTION_ID)) {
            auto connection = getPropertyFromTree<ConnectionLine>(child, ConnectionLine::IDs::ConnectionLineObject);
            for (auto s : selected.getItemArray()) {
                if (auto e = dynamic_cast<Effect*>(s.get())) {
                    if (e->hasConnection(connection))
                    {
                        connectionsToChange.add(connection);
                    }
                }
            }
        }
    }

    // Add effects to new
    for (auto s : selected.getItemArray()) {
        if (auto e = dynamic_cast<Effect*>(s.get())) {
            // Adjust new effect size
            newE->setBounds(newE->getBounds().getUnion(e->getBounds().expanded(20)));

            //TODO from here
            auto newEffectPos = newE->getLocalPoint(e->getParentComponent(), e->getPosition());

            // Reassign parent //TODO reassign() + pos change and shit
            e->getTree().getParent().removeChild(e->getTree(), &undoManager); // Auto-disconnects connections
            newEffectTree.appendChild(e->getTree(), &undoManager);

            e->setTopLeftPosition(newEffectPos);
        }
    }
    // Add new connections
    for (auto c : connectionsToChange) {
        if (! selected.getItemArray().contains(dynamic_cast<Effect*>(c->getOutPort()->getParentComponent()))){
            // Create connection going from this to outPort parent
            // Create Output port and internal
            auto newPort = newE->addPort(Effect::getDefaultBus(), false);
            // Connect internal
            auto inConnection = newConnection(c->getInPort(), newPort->internalPort);
            newEffectTree.appendChild(inConnection, &undoManager);
            // Connect external
            auto outConnection = newConnection(c->getOutPort(), newPort);
            newEffectTree.appendChild(outConnection, &undoManager);
        } else if (! selected.getItemArray().contains(dynamic_cast<Effect*>(c->getInPort()->getParentComponent()))) {
            // Create connection going from inPort parent to this
            // Create Input port and internal
            auto newPort = newE->addPort(Effect::getDefaultBus(), true);
            // Connect internal
            auto inConnection = newConnection(c->getOutPort(), newPort->internalPort);
            newEffectTree.appendChild(inConnection, &undoManager);
            // Connect external
            auto outConnection = newConnection(c->getInPort(), newPort);
            newEffectTree.appendChild(outConnection, &undoManager);
        } else {
            auto connection = newConnection(c->getInPort(), c->getOutPort());
            tree.appendChild(connection, &undoManager);
        }
    }

    selected.deselectAll();
}
*/

//======================================================================================

Effect::Effect() : MenuItem(2)
{
    addAndMakeVisible(resizer);
    resizer.setAlwaysOnTop(true);
}

void Effect::setupTitle() {
    Font titleFont(20, Font::FontStyleFlags::bold);
    title.setFont(titleFont);

    title.setEditable(true);
    title.setText(getName(), dontSendNotification);
    title.setBounds(25, 20, 200, title.getFont().getHeight());

    title.onTextChange = [=]{
        // Name change undoable action
        undoManager.beginNewTransaction("Name change to: " + title.getText(true));
        setName(title.getText(true));
    };

    addAndMakeVisible(title);

    if (editMode && appState != loading) {
        //title.grabKeyboardFocus();
        title.setWantsKeyboardFocus(true);
        title.showEditor();
    }
}

void Effect::setupMenu() {
    PopupMenu::Item toggleEditMode("Toggle Edit Mode");
    PopupMenu::Item changeEffectImage("Change Effect Image..");

    std::unique_ptr<PopupMenu> portSubMenu = std::make_unique<PopupMenu>();
    PopupMenu::Item portSubMenuItem("Add Port..");



    toggleEditMode.setAction([=] {
        setEditMode(!editMode);
    });

    changeEffectImage.setAction([=] {
        FileChooser imgChooser ("Select Effect Image..",
                                File::getSpecialLocation (File::userHomeDirectory),
                                "*.jpg;*.png;*.gif");
        if (imgChooser.browseForFileToOpen())
        {
            image = ImageFileFormat::loadFrom(imgChooser.getResult());
        }
    });

    portSubMenu->addItem("Add Input Port", [=]() {
        addPort(getDefaultBus(), true);
        resized();
    });
    portSubMenu->addItem("Add Output Port", [=](){
        addPort(getDefaultBus(), false);
        resized();
    });
    portSubMenuItem.subMenu = std::move(portSubMenu);


    addMenuItem(editMenu, toggleEditMode);
    addMenuItem(menu, toggleEditMode);
    addMenuItem(editMenu, changeEffectImage);
    addMenuItem(editMenu, portSubMenuItem);

    //todo move to class that can access
    /*if (!isIndividual()) {
        PopupMenu::Item saveEffect("Save Effect");
        saveEffect.setAction([=]() {
            auto saveTree = storeEffect(tree);

            saveTree.setProperty("editMode", false, nullptr);

            if (saveTree.isValid()) {
                EffectLoader::saveEffect(saveTree);
                getParentComponent()->postCommandMessage(0);
            } else {
                std::cout << "invalid, mothafucka." << newLine;
            }
        });

        menu.addItem(saveEffect);
        editMenu.addItem(saveEffect);
    }*/

    /*PopupMenu parameterSubMenu;
    parameterSubMenu.addItem("Add Slider", [=] () {
        undoManager.beginNewTransaction("Add slider parameter");
        auto newParam = createParameter(new MetaParameter("New Parameter"));
        newParam.setProperty("x", getMouseXYRelative().getX(), nullptr);
        newParam.setProperty("y", getMouseXYRelative().getY(), nullptr);
        tree.appendChild(newParam, &undoManager);
        loadParameter(newParam);
    });
    editMenu.addSubMenu("Add Parameter..", parameterSubMenu);
*/
}

Effect::~Effect()
{
    if (node != nullptr) {
        std::cout << "Reference count: " << node->getReferenceCount() << newLine;
        while (node->getReferenceCount() > 1) {
            node->decReferenceCount();
        }
    }
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
        bool isInput = i < numInputBuses;
        // Get bus from processor
        auto bus = isInput ? processor->getBus(true, i) :
                   processor->getBus(false, i - numInputBuses);
        // Check channel number - if 0 ignore
        if (bus->getNumberOfChannels() == 0)
            continue;
        // Create port - giving audiochannelset info and isInput bool
        addPort(bus, isInput);
    }

    // Save AudioProcessorParameterGroup
    parameters = &processor->getParameterTree();
}


ValueTree Effect::createParameter(AudioProcessorParameter *param) {
    ValueTree parameterTree(PARAMETER_ID);
    parameterTree.setProperty("name", param->getName(30), nullptr);

    return parameterTree;
}

/**
 * Loads parameter from value tree data
 * @param parameterData This stupid motherfucker is a reference because of the effect tree loading shit.
 * Fucking hell man. I'm stuck doing this 9-5 and I have no friends. Every day I
 * think of ways to kill myself. Fuck all this shit man.
 * @return parameter created
 */
Parameter::Ptr Effect::loadParameter(ValueTree parameterData) {
    auto name = parameterData.getProperty("name");

    AudioProcessorParameter* param = nullptr;

    if (isIndividual()) {
        for(auto p : getParameters(false)) {
            if (p->getName(30).compare(name) == 0) {
                param = p;
            }
        }
    } else {
        param = new MetaParameter(name);
        audioGraph->addParameter(param);
    }

    Parameter::Ptr parameter = new Parameter(param);
    parameterArray.add(parameter);

    if (isIndividual()) {
        parameter->setEditMode(false);
    } else {
        parameter->setEditMode(editMode);
    }

    addAndMakeVisible(parameter.get());
    addAndMakeVisible(parameter->getPort(true));

    // Set position

    int x = parameterData.getProperty("x");
    int y = parameterData.getProperty("y");

    // Set value
    if (parameterData.hasProperty("value")) {
        float value = parameterData.getProperty("value");
        param->setValueNotifyingHost(value);
    }

    parameterData.setProperty(Parameter::IDs::parameterObject, parameter.get(), nullptr);

    parameter->setTopLeftPosition(x, y);


    // if has connection then connect


    return parameter;
}


/**
 * Get the port at the given location, if there is one
 * @param pos relative to this component (no conversion needed here)
 * @return nullptr if no match, ConnectionPort* if found
 */
ConnectionPort* Effect::checkPort(Point<int> pos) {
    for (auto p : inputPorts) {
        if (p->contains(p->getLocalPoint(this, pos)))
        {
            return p;
        } else if (p->getInternalConnectionPort()->contains(
                p->getInternalConnectionPort()->getLocalPoint(this, pos)))
        {
            return p->getInternalConnectionPort().get();
        }
    }
    for (auto p : outputPorts) {
        if (p->contains(p->getLocalPoint(this, pos)))
        {
            return p;
        } else if (p->getInternalConnectionPort()->contains(p->getInternalConnectionPort()->getLocalPoint(this, pos)))
        {
            return p->getInternalConnectionPort().get();
        }
    }
    return nullptr;
}

void Effect::setEditMode(bool isEditMode) {
    if (isIndividual())
        return;

    // Turn on edit mode
    if (isEditMode) {
        // Set child effects and connections to editable
        for (int i = 0; i < getNumChildComponents(); i++) {
            auto c = getChildComponent(i);

            c->toFront(false);
            c->setInterceptsMouseClicks(true, true);

            if (dynamic_cast<Effect*>(c)) {
                c->setVisible(true);
            } else if (dynamic_cast<ConnectionLine*>(c)) {
                c->setVisible(true);
            }
        }

        for (auto l : {inputPorts, outputPorts}) {
            for (auto p : l) {
                p->setColour(0, Colours::whitesmoke);
                p->internalPort->setVisible(true);
            }
        }

        for (auto c : getChildren()) {
            if (auto p = dynamic_cast<Parameter*>(c)) {
                p->setEditMode(true);
            }
        }

        title.setMouseCursor(MouseCursor::IBeamCursor);
        title.setInterceptsMouseClicks(true, true);

        title.setColour(title.textColourId, Colours::whitesmoke);
    }

    // Turn off edit mode
    else if (! isEditMode) {

//        bool hideEffects //= image.isValid();
        //std::cout << "Image valid: " << hideEffects << newLine;

        // Make child effects and connections not editable
        for (int i = 0; i < getNumChildComponents(); i++) {
            auto c = getChildComponent(i);

            c->toBack();
            c->setInterceptsMouseClicks(false, false);
            if (dynamic_cast<Effect*>(c)) {
                c->setVisible(false);
            } else if (dynamic_cast<ConnectionLine*>(c)) {
                c->setVisible(false);
            }
        }

        for (auto l : {inputPorts, outputPorts}) {
            for (auto p : l) {
                p->setColour(0, Colours::black);
                p->internalPort->setVisible(false);
            }
        }

        for (auto c : getChildren()) {
            if (auto p = dynamic_cast<Parameter*>(c)) {
                p->setEditMode(false);
                p->setInterceptsMouseClicks(false, true);
            }
        }

        title.setMouseCursor(getMouseCursor());
        title.setInterceptsMouseClicks(false,false);
        title.setColour(title.textColourId, Colours::black);
    }

    editMode = isEditMode;
    repaint();
}


/*
Parameter& Effect::addParameterFromProcessorParam(AudioProcessorParameter *param)
{
    Parameter* parameterGui;

    auto paramName = param->getName(30).trim();

    ValueTree parameter(PARAMETER_ID);

    parameterGui = new Parameter(param);
    parameter.setProperty(Parameter::IDs::parameterObject, parameterGui, nullptr);

    tree.appendChild(parameter, &undoManager);

    return *parameterGui;
}
*/


AudioPort::Ptr Effect::addPort(AudioProcessor::Bus *bus, bool isInput) {
    auto p = isInput ?
             inputPorts.add(new AudioPort(isInput)) :
             outputPorts.add(new AudioPort(isInput));
    p->bus = bus;
    if (editMode){
        p->setColour(p->portColour, Colours::whitesmoke);
    }

    addAndMakeVisible(p);

    if (!isIndividual()) {
        addChildComponent(p->internalPort.get());
        Point<int> d;
        d = isInput ? Point<int>(50, 0) : Point<int>(-50, 0);

        p->internalPort->setColour(p->portColour, Colours::whitesmoke);
        p->internalPort->setCentrePosition(getLocalPoint(p, p->centrePoint + d));
        p->internalPort->setVisible(true);
    } else {
        p->internalPort->setVisible(false);
    }

    return p;
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



/*

void Effect::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    std::cout << "VT property changed: " << property.toString() << newLine;
    if (treeWhosePropertyHasChanged == tree) {
        if (property == IDs::x) {
            int x = treeWhosePropertyHasChanged.getProperty(IDs::x);
            setTopLeftPosition(x, getY());
        } else if (property == IDs::y) {
            int y = treeWhosePropertyHasChanged.getProperty(IDs::y);
            setTopLeftPosition(getX(), y);
        } else if (property == IDs::w) {
            if (undoManager.isPerformingUndoRedo()) {
                int w = tree.getProperty(IDs::w);
                setSize(w, getHeight());
            }
        } else if (property == IDs::h) {
            if (undoManager.isPerformingUndoRedo()) {
                int h = tree.getProperty(IDs::h);
                setSize(h, getWidth());
            }
        } else if (property == IDs::name) {
            auto e = getFromTree<Effect>(treeWhosePropertyHasChanged);
            if (e != nullptr) {
                auto newName = treeWhosePropertyHasChanged.getProperty(IDs::name);

                e->setName(newName);
                e->title.setText(newName, sendNotificationAsync);
            }
        } else if (property == EffectTreeBase::IDs::effectTreeBase) {
            std::cout << "bro wut" << newLine;
        }
    }
    Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}

void Effect::valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) {
    std::cout << "Parent changed" << newLine;
    if (treeWhoseParentHasChanged == tree) {

    }
}
*/

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

    int i = 0;
    for (auto c : getChildren()) {
        if (auto parameter = dynamic_cast<Parameter*>(c)) {
            auto paramPort = parameter->getPort(true);

            paramPort->setCentrePosition(100 + i * 50, getHeight() - 5);
            i++;
        }
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

int Effect::getPortID(const ConnectionPort *port) {
    if (auto p = dynamic_cast<const AudioPort*>(port)) {
        if (inputPorts.contains(p)) {
            return inputPorts.indexOf(p);
        } else if (outputPorts.contains(p)) {
            return inputPorts.size() + outputPorts.indexOf(p);
        }
    } else if (auto p = dynamic_cast<const InternalConnectionPort*>(port)) {
        for (int i = 0; i < inputPorts.size(); i++) {
            if (inputPorts[i]->internalPort == p) {
                return i;
            }
        }
        for (int i = 0; i < outputPorts.size(); i++) {
            if (outputPorts[i]->internalPort == p) {
                return inputPorts.size() + i;
            }
        }
    }
    return -1;
}

ConnectionPort* Effect::getPortFromID(const int id, bool internal) {
    AudioPort* port;
    if (id < inputPorts.size()) {
        port = inputPorts[id].get();
    } else {
        port = outputPorts[id - inputPorts.size()].get();
    }

    if (internal) {
        return port->getInternalConnectionPort().get();
    } else {
        return port;
    }
}


bool Effect::hasProcessor(AudioProcessor *processor) {
    return processor == this->processor;
}

/*void Effect::updateEffectProcessor(AudioProcessor *processorToUpdate, ValueTree treeToSearch) {
    for (int i = 0; i < treeToSearch.getNumChildren(); i++) {
        if (auto e = getFromTree<Effect>(treeToSearch.getChild(i))) {
            if (e->hasProcessor(processorToUpdate)) {
                // update processor gui
                if (processorToUpdate->getMainBusNumInputChannels() > e->inputPorts.size()) {
                    auto numPortsToAdd = processorToUpdate->getMainBusNumInputChannels() - e->inputPorts.size();
                    for (int j = 0; j < numPortsToAdd; j++) {
                        e->addPort(e->getDefaultBus(), true);
                    }
                }
                if (processorToUpdate->getMainBusNumOutputChannels() < e->outputPorts.size()) {
                    auto numPortsToAdd = processorToUpdate->getMainBusNumOutputChannels() - e->outputPorts.size();
                    for (int j = 0; j < numPortsToAdd; j++) {
                        e->addPort(e->getDefaultBus(), false);
                    }
                }
            } else {
                //recurse
                updateEffectProcessor(processorToUpdate, treeToSearch.getChild(i));
            }
        }
    }
}*/

bool Effect::hasPort(const ConnectionPort *port) {
    if (auto p = dynamic_cast<const AudioPort*>(port)) {
        return (inputPorts.contains(p) || outputPorts.contains(p));
    } else if (auto p = dynamic_cast<const InternalConnectionPort*>(port)) {
        for (auto list : {inputPorts, outputPorts}) {
            for (auto portToCheck : list) {
                if (portToCheck->internalPort == p) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Effect::hasConnection(const ConnectionLine *line) {
    return (hasPort(line->getOutPort().get())
        || hasPort(line->getInPort().get()));
}

/*ValueTree Effect::storeParameters() {
    if (parameters == nullptr) {
        return ValueTree();
    } else {
        ValueTree parameterValues("parameters");
        for (auto p : parameters->getParameters(false)) {
            parameterValues.setProperty(p->getName(30), p->getValue(), nullptr);
        }
        return parameterValues;
    }
}

void Effect::loadParameters(ValueTree parameterValues) {
    auto parameterList = parameters->getParameters(false);
    for (auto p : parameters->getParameters(false)) {
        if (parameterValues.hasProperty(p->getName(30))){
            float val = parameterValues.getProperty(p->getName(30));
            p->setValueNotifyingHost(val);
        }
    }
}*/

Parameter *Effect::getParameterForPort(ParameterPort *port) {
    for (auto c : getChildren()) {
        if (auto param = dynamic_cast<Parameter*>(c)) {
            if (param->getPort(true) == port) {
                return param;
            }
        }
    }
    return nullptr;
}

Array<AudioProcessorParameter *> Effect::getParameters(bool recursive) {
    if (parameters == nullptr) {
        return Array<AudioProcessorParameter *>();
    } else {
        return parameters->getParameters(recursive);
    }
}

void Effect::setNode(AudioProcessorGraph::Node::Ptr node) {
    this->node = node;
}

Array<Parameter*> Effect::getParameterChildren() {
    Array<Parameter*> list;
    for (auto c : getChildren()) {
        if (auto p = dynamic_cast<Parameter*>(c)) {
            list.add(p);
        }
    }
    return list;
}


/**
 * Returns a list of pointers to ports, according to isInput parameter.
 * @param isInput
 * -1 for both input and output ports
 * 0 for output ports
 * 1 for input ports
 * @return list of pointers to ports.
 */
Array<ConnectionPort *> Effect::getPorts(int isInput) {
    Array<ConnectionPort*> list;

    if (isInput <= 0) {
        for (auto p : outputPorts) {
            list.add(p);
        }
    }

    if (isInput != 0) {
        for (auto p : inputPorts) {
            list.add(p);
        }
    }

    return list;
}

void Effect::mouseDrag(const MouseEvent &event) {
    Component::mouseDrag(event);
    getParentComponent()->mouseDrag(event);
}

void Effect::mouseDown(const MouseEvent &event) {
    SelectHoverObject::mouseDown(event);
    getParentComponent()->mouseDown(event);
}

void Effect::mouseUp(const MouseEvent &event) {
    SelectHoverObject::mouseUp(event);
    getParentComponent()->mouseUp(event);
}



/*

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


ReferenceCountedArray<ConnectionLine> ConnectionVar::fromVarArray(const var &v) {
    Array<var>* varArray = v.getArray();
    ReferenceCountedArray<ConnectionLine> array;
    for (auto i : *varArray) {
        array.add(dynamic_cast<ConnectionLine*>(i.getObject()));
    }

    return array;
}

var ConnectionVar::toVarArray(const ReferenceCountedArray<ConnectionLine> &t) {
    Array<var> varArray;
    for (auto i : t) {
        varArray.add(i);
    }

    return varArray;
}

ConnectionLine::Ptr ConnectionVar::fromVar(const var &v) {
    return dynamic_cast<ConnectionLine*>(v.getObject());
}

var* ConnectionVar::toVar(const ConnectionLine::Ptr &t) {
    //var v = t.get();
    return reinterpret_cast<var *>(t.get());
}
*/

