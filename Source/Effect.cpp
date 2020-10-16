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
#include "Ports.h"
#include "EffectPositioner.h"
#include "EffectTree.h"
#include "Audio-Effects/ProcessorLib.h"


// Static members
EffectBase::AppState EffectBase::appState = neutral;
//AudioProcessorGraph* EffectBase::audioGraph = nullptr;
//AudioProcessorPlayer* EffectBase::processorPlayer = nullptr;
AudioDeviceManager* EffectBase::deviceManager = nullptr;
//ConnectionGraph* EffectBase::connectionGraph = nullptr;
UndoManager EffectBase::undoManager;

const Identifier Effect::IDs::x = "x";
const Identifier Effect::IDs::y = "y";
const Identifier Effect::IDs::w = "w";
const Identifier Effect::IDs::h = "h";
const Identifier Effect::IDs::processorID = "processor";
const Identifier Effect::IDs::initialised = "initialised";
const Identifier Effect::IDs::name = "name";
const Identifier Effect::IDs::connections = "connections";
const Identifier Effect::IDs::EFFECT_ID = "effect";
const Identifier Effect::IDs::editMode = "editMode";

EffectBase* EffectBase::effectScene = nullptr;


EffectBase::EffectBase() {

}

bool EffectBase::connectParameters(const ConnectionLine &connectionLine) {
    auto inPort = dynamic_cast<ParameterPort*>(connectionLine.getInPort());
    auto outPort = dynamic_cast<ParameterPort*>(connectionLine.getOutPort());

    auto inEffect = dynamic_cast<Effect*>(inPort->getParentEffect());
    auto outEffect = dynamic_cast<Effect*>(outPort->getParentEffect());

    jassert(inEffect != nullptr && outEffect != nullptr);

    auto inParam = inEffect->getParameterForPort(inPort);
    auto outParam = dynamic_cast<Parameter*>(outEffect->getParameterForPort(outPort));

    if (outParam->isOutput()) {
        inParam->connect(outParam);
    } else {
        outParam->connect(inParam);
    }
    return true; //outParam->isConnected();
}

void EffectBase::disconnectParameters(const ConnectionLine &connectionLine) {
    auto port1 = dynamic_cast<ParameterPort*>(connectionLine.getOutPort());
    auto port2 = dynamic_cast<ParameterPort*>(connectionLine.getInPort());

    jassert(port1 != nullptr && port2 != nullptr);

    auto parameter1 = dynamic_cast<Parameter*>(dynamic_cast<Effect*>(
            port1->getParentEffect())->getParameterForPort(port1));
    auto parameter2 = dynamic_cast<Parameter*>(dynamic_cast<Effect*>(
            port2->getParentEffect())->getParameterForPort(port2));

    jassert(parameter1 != nullptr && parameter2 != nullptr);

    parameter1->disconnect(parameter2);
}


bool EffectBase::connectAudio(const ConnectionLine& connectionLine) {
    return ConnectionGraph::getInstance()->addConnection(&connectionLine);

}

void EffectBase::disconnectAudio(const ConnectionLine &connectionLine) {
    ConnectionGraph::getInstance()->removeConnection(&connectionLine);

}

void EffectBase::mouseDown(const MouseEvent &event) {
    //todo move this to port class

    // Handle Connection
    if (auto port = dynamic_cast<ConnectionPort_old*>(event.originalComponent)) {
        dragLine = new ConnectionLine();

        dragLine->setPort(port);

        auto parent = port->getDragLineParent();

        parent->addAndMakeVisible(dragLine);
        parent->addMouseListener(dragLine, true);
    }
}


Array<ConnectionLine *> EffectBase::getConnectionsToThis(bool isInputConnection, ConnectionLine::Type connectionType) {
    Array<ConnectionLine*> array;
    for (auto c : getParentComponent()->getChildren()) {
        if (auto line = dynamic_cast<ConnectionLine*>(c)) {
            if (line->type == connectionType) {
                if (isInputConnection && isParentOf(line->getInPort())) {
                    array.add(line);
                } else if (!isInputConnection && isParentOf(line->getOutPort())) {
                    array.add(line);
                }
            }
        }
    }

    return array;
}

Array<ConnectionLine *> EffectBase::getConnectionsToThis() {
    Array<ConnectionLine*> array;

    if (getParentComponent() == nullptr) {
        return array;
    }

    for (auto c : getParentComponent()->getChildren()) {
        if (auto line = dynamic_cast<ConnectionLine*>(c)) {
            if (isParentOf(line->getInPort()) || isParentOf(line->getOutPort())) {
                array.add(line);
            }
        }
    }

    return array;
}


ConnectionPort_old* EffectBase::getPortFromID(String portID) {
    auto ref = Identifier(portID);

    if (auto p = findChildWithID(ref)) {
        return dynamic_cast<ConnectionPort_old*>(p);
    }

    for (auto child : getChildren()) {
        if (auto effect = dynamic_cast<Effect*>(child)) {
            for (auto p : effect->getParameterChildren()) {
                if (auto port = p->getPortWithID(portID)) {
                    return port;
                }
            }

            if (auto p = child->findChildWithID(ref)) {
                return dynamic_cast<ConnectionPort_old*>(p);
            }
        }
        if (auto param = dynamic_cast<Parameter*>(child)) {
            auto port = param->getPort(false);
            if (port->getComponentID() == portID) {
                return port;
            }
            port = param->getPort(true);
            if (port->getComponentID() == portID) {
                return port;
            }
        }
    }
}

void EffectBase::handleCommandMessage(int commandId) {
    if (commandId == 9) {
        undoManager.beginNewTransaction();
    } else {
        getParentComponent()->handleCommandMessage(commandId);
    }
    Component::handleCommandMessage(commandId);
}

Array<ConnectionLine *> EffectBase::getConnectionsInside() {
    Array<ConnectionLine *> array;
    for (auto c : getChildren()) {
        if (auto line = dynamic_cast<ConnectionLine*>(c)) {
            array.add(line);
        }
    }
    return array;
}

/*
void EffectBase::createGroupEffect() {
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

Effect::Effect(String name, int processorID, bool editMode,
               int x, int y, int w, int h) :
               MenuItem(2),
               masterReference(this)
{
    //=======================================================================
    // Name
    setName(name);

    //=======================================================================
    // Processor ID
    if (processorID > 0) {
        // Individual Effect
        std::unique_ptr<AudioProcessor> newProcessor;

        newProcessor = ProcessorLib::createProcessor(processorID);

        auto node = ConnectionGraph::getInstance()->addNode(move(newProcessor));
        setNode(node);
        // Create from node:
        setProcessor(node->getProcessor());
    }

    //=======================================================================
    // EditMode
    setEditMode(editMode);

    //=======================================================================
    // Bounds
    setBounds(x, y, h, w);

    addAndMakeVisible(resizer);
    resizer.setAlwaysOnTop(true);

    parameterFlexBox.flexDirection = FlexBox::Direction::column;
    parameterFlexBox.alignContent = FlexBox::AlignContent::flexStart;
    parameterFlexBox.alignItems = FlexBox::AlignItems::center;
    parameterFlexBox.justifyContent = FlexBox::JustifyContent::center;

    paramPortsFlexBox.flexDirection = FlexBox::Direction::row;
    paramPortsFlexBox.alignItems = FlexBox::AlignItems::center;
    paramPortsFlexBox.alignContent = FlexBox::AlignContent::center;
    paramPortsFlexBox.justifyContent = FlexBox::JustifyContent::center;
    paramPortsFlexBox.flexWrap = FlexBox::Wrap::wrap;

    internalInputPortFlexBox.flexDirection = internalOutputPortFlexBox.flexDirection =
            outputPortFlexBox.flexDirection = inputPortFlexBox.flexDirection = FlexBox::Direction::column;
    internalInputPortFlexBox.justifyContent = internalOutputPortFlexBox.justifyContent =
        outputPortFlexBox.justifyContent = inputPortFlexBox.justifyContent = FlexBox::JustifyContent::center;

    title.setBounds(30,30,200, title.getFont().getHeight());

    //===================================================
    //
}

Effect::~Effect()
{
    masterReference = nullptr;
}

void Effect::setupTitle() {
    Font titleFont(20, Font::FontStyleFlags::bold);
    title.setFont(titleFont);

    title.setEditable(true);
    title.setText(getName(), dontSendNotification);

    addAndMakeVisible(title);
    title.setBounds(25, 20, 200, title.getFont().getHeight());

    title.onTextChange = [=]{
        // Name change undoable action
        undoManager.beginNewTransaction("Name change to: " + title.getText(true));
        setName(title.getText(true));
    };


    if (editMode && appState != loading) {
        //title.grabKeyboardFocus();
        title.setWantsKeyboardFocus(true);
        title.showEditor();
    }
}

void Effect::setupMenu() {
    PopupMenu::Item toggleEditMode("Toggle Edit Mode");
    PopupMenu::Item changeEffectImage("Change Effect Image..");

    std::unique_ptr<PopupMenu> saveEffectMenu = std::make_unique<PopupMenu>();
    PopupMenu::Item saveSubMenuItem("Save Effect..");

    PopupMenu::Item saveEffectToLibItem("Save to Library..");
    saveEffectToLibItem.action = [=] { getParentComponent()->handleCommandMessage(1); };
    saveEffectMenu->addItem(saveEffectToLibItem);

    PopupMenu::Item saveEffectExportItem("Export to file..");
    saveEffectExportItem.action = [=] { getParentComponent()->handleCommandMessage(2); };
    saveEffectMenu->addItem(saveEffectExportItem);

    saveSubMenuItem.subMenu = std::move(saveEffectMenu);

    addMenuItem(menu, saveSubMenuItem);
    addMenuItem(editMenu, saveSubMenuItem);

    std::unique_ptr<PopupMenu> portSubMenu = std::make_unique<PopupMenu>();
    PopupMenu::Item portSubMenuItem("Add Port..");

    std::unique_ptr<PopupMenu> parameterSubMenu = std::make_unique<PopupMenu>();
    PopupMenu::Item addParamSubMenuItem("Add Parameter..");

    // Actions

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

    portSubMenu->addItem("Input Port", [=]() {
        addPort(ConnectionGraph::getInstance()->getDefaultBus(), true);
        resized();
    });
    portSubMenu->addItem("Output Port", [=](){
        addPort(ConnectionGraph::getInstance()->getDefaultBus(), false);
        resized();
    });
    portSubMenuItem.subMenu = std::move(portSubMenu);

    parameterSubMenu->addItem("Slider", [=] () {
        undoManager.beginNewTransaction("Add slider parameter");

        auto parameter = new SliderParameter(nullptr);
        parameter->setParentEditMode(true);
        addAndMakeVisible(parameter);
        repositionInternals();
        //parameter->setCentrePosition(getMouseXYRelative());
    });
    parameterSubMenu->addItem("Selection Box", [=] () {
        undoManager.beginNewTransaction("Add combo parameter");

        auto parameter = new ComboParameter(nullptr);
        parameter->setParentEditMode(true);
        addAndMakeVisible(parameter);
        repositionInternals();
        //parameter->setCentrePosition(getMouseXYRelative());
    });
    parameterSubMenu->addItem("Toggle Button", [=] () {
        undoManager.beginNewTransaction("Add button parameter");

        auto parameter = new ButtonParameter(nullptr);
        parameter->setParentEditMode(true);
        addAndMakeVisible(parameter);
        repositionInternals();
        //parameter->setCentrePosition(getMouseXYRelative());
    });
    addParamSubMenuItem.subMenu = std::move(parameterSubMenu);

    addMenuItem(editMenu, toggleEditMode);
    addMenuItem(menu, toggleEditMode);
    addMenuItem(editMenu, changeEffectImage);
    addMenuItem(editMenu, portSubMenuItem);
    addMenuItem(editMenu, addParamSubMenuItem);

}

// Processor hasEditor? What to do if processor is a predefined plugin
void Effect::setProcessor(AudioProcessor *processor) {
    this->processor = processor;

    // Processor settings (how best to do this?)
    /*node->getProcessor()->setPlayConfigDetails(
            processor->getTotalNumInputChannels(),
            processor->getTotalNumOutputChannels(),
            audioGraph->getSampleRate(),
            audioGraph->getBlockSize());*/

    // Save AudioProcessorParameterGroup
    parameters = &processor->getParameterTree();
}

void Effect::setEditMode(bool isEditMode) {
    editMode = isEditMode;

    // Turn on edit mode
    if (isEditMode) {
        // Set child effects and connections to editable
        for (int i = 0; i < getNumChildComponents(); i++) {
            auto c = getChildComponent(i);

            //c->toFront(false);
            c->setInterceptsMouseClicks(true, true);

            if (dynamic_cast<Effect*>(c)) {
                c->setVisible(true);
            } else if (dynamic_cast<ConnectionLine*>(c)) {
                c->setVisible(true);
            }
        }

        //todo change singular colour
        for (auto c : getChildren()) {
            if (auto p = dynamic_cast<AudioPort*>(c)) {
                p->setColour(0, Colours::whitesmoke);
                p->internalPort->setVisible(true);
            }
        }

        for (auto c : getChildren()) {
            if (auto p = dynamic_cast<Parameter*>(c)) {
                p->setParentEditMode(true);
            }
        }

        title.setMouseCursor(MouseCursor::IBeamCursor);
        title.setInterceptsMouseClicks(true, true);

        title.setColour(title.textColourId, Colours::whitesmoke);

        expandToFitChildren();
    }

    // Turn off edit mode
    else if (! isEditMode) {

//        bool hideEffects //= image.isValid();
        //std::cout << "Image valid: " << hideEffects << newLine;

        // Make child effects and connections not editable
        for (int i = 0; i < getNumChildComponents(); i++) {
            auto c = getChildComponent(i);

            if (dynamic_cast<Resizer*>(c)) {
                continue;
            }

            //c->toBack();
            //c->setInterceptsMouseClicks(false, false);
            if (dynamic_cast<Effect*>(c)) {
                c->setVisible(false);
            } else if (dynamic_cast<ConnectionLine*>(c)) {
                c->setVisible(false);
            }
        }

        for (auto c : getChildren()) {
            if (auto p = dynamic_cast<AudioPort*>(c)) {
                p->setColour(0, Colours::black);
                p->internalPort->setVisible(false);
            }
        }

        for (auto c : getChildren()) {
            if (auto p = dynamic_cast<Parameter*>(c)) {
                p->setParentEditMode(false);
                //p->setInterceptsMouseClicks(false, true);
            }
        }

        title.setMouseCursor(getMouseCursor());
        title.setInterceptsMouseClicks(false,false);
        title.setColour(title.textColourId, Colours::black);
    }

    repositionInternals();
    //repaint();
}


AudioPort::Ptr Effect::addPort(AudioProcessor::Bus *bus, bool isInput) {
    auto p = new AudioPort(isInput);

    p->bus = bus;
    if (editMode){
        p->setColour(p->portColour, Colours::whitesmoke);
    }

    if (!isIndividual()) {
        addChildComponent(p->internalPort.get());

        p->internalPort->setColour(p->portColour, Colours::whitesmoke);
        p->internalPort->setVisible(true);
    } else {
        p->internalPort->setVisible(false);
    }

    addPort(p);

    return p;
}

ConnectionPort_old* Effect::getEndPort(ConnectionPort_old* port) {
    if (port == nullptr) return nullptr;
    
    if (! port->isConnected()) {
        return nullptr;
    } else if (dynamic_cast<Effect*>(port->getParentEffect())->isIndividual()) {
        return port;
    } else {
        // Recurse through effects/ports until we reach a processor
        auto nextPort = port->getLinkedPort();
        if (nextPort != nullptr && nextPort->isConnected()) {
            return getEndPort(nextPort->getOtherPort());
        } else {
            return nullptr;
        }
    }
}

ConnectionPort_old *Effect::getNextPort(ConnectionPort_old *port, bool stopAtProcessor) {

    if (port->isConnected()) {
        auto otherPort = port->getOtherPort();
        auto nextEffect = dynamic_cast<Effect *>(otherPort->getParentEffect());

        if (nextEffect->isIndividual()) {
            if (stopAtProcessor) {
                return otherPort;
            } else {
                auto ports = nextEffect->getPorts(port->isInput);
                auto doesConnectionContinue = false;
                for (auto p : ports) {
                    doesConnectionContinue |= p->isConnected();
                }
                return doesConnectionContinue ? ports.getFirst() : otherPort;
            }
        } else {
            return otherPort->getLinkedPort();
        }
    } else {
        return nullptr;
    }

}


Array<Effect *> Effect::getFullConnectionEffects(ConnectionPort_old *port, bool includeParent) {
    Array<Effect *> array;

    auto nextPort = getNextPort(port, false); //todo array

    if (nextPort != nullptr) {
        auto nextEffect = dynamic_cast<Effect *>(nextPort->getParentEffect());

        // Add next except if includeParent == false && nextEffect is parent of current
        if (! nextEffect->isParentOf(port->getParentEffect()) || includeParent) {
            array.add(nextEffect);
        }

        // If the nextPort is facing this port, then it's the end of the line.
        if (nextPort->isInput == port->isInput) {
            // Else, we recurse.
            array.addArray(nextEffect->getFullConnectionEffects(nextPort));
        }
    }

    return array;
}


Array<Effect *> Effect::getFullConnectionEffectsInside() {
    Array<Effect *> array;

    for (auto port : getPorts(true)) {
        auto insidePort = port->getLinkedPort();
        ConnectionPort_old* nextPort = getNextPort(insidePort);

        while (nextPort != nullptr && nextPort->getParentEffect() != this) {
            array.add(dynamic_cast<Effect*>(nextPort->getParentEffect()));

            nextPort = getNextPort(insidePort);
        }
    }

    return Array<Effect *>();
}

Array<ConnectionLine *> Effect::getConnectionsUntilEnd(ConnectionPort_old* port) {
    Array<ConnectionLine*> array;
    while (! dynamic_cast<Effect*>(port->getParentEffect())->isIndividual()) {
        port = getNextPort(port);
        if (port->isConnected()) {
            for (auto c : dynamic_cast<EffectBase *>(port->getDragLineParent())->getConnectionsInside()) {
                if ((c->getInPort() == port && c->getOutPort() == port->getOtherPort())
                || (c->getOutPort() == port && c->getInPort() == port->getOtherPort())) //todo checkports function
                {
                    array.add(c);
                }
            }
        }
    }
    return array;

    /*if (! port->isConnected()) {
        return nullptr;
    } else if (isIndividual()) {
        return port;
    } else {
        return getNextPort(port);
    }

    return Array<ConnectionLine *>();*/
}

AudioProcessorGraph::NodeID Effect::getNodeID() const {
    return node->nodeID;
}


void Effect::resized() {
    inputPortFlexBox.performLayout(Rectangle<int>(10, 18, 60, getHeight() - 18));
    outputPortFlexBox.performLayout(Rectangle<int>(getWidth() - 90, 18, 60, getHeight() - 18));
    internalInputPortFlexBox.performLayout(Rectangle<int>(80, 30, 60, getHeight()));
    internalOutputPortFlexBox.performLayout(Rectangle<int>(getWidth() - 120, 30, 60, getHeight()));

    if (editMode) {
        parameterFlexBox.performLayout(Rectangle<int>(20, getHeight() - 100, getWidth() - 40, 100));
        paramPortsFlexBox.performLayout(Rectangle<int>(5, getHeight() - 30
                , getWidth() - 5, 30));
    } else {
        parameterFlexBox.performLayout(Rectangle<int>((inputPortFlexBox.items.size() > 0) ? 70 : 15
                , 50, (outputPortFlexBox.items.size() > 0) ? getWidth() - 60 : getWidth() - 15
                , getHeight() - 30));
        paramPortsFlexBox.performLayout(Rectangle<int>(5, getHeight() - 40
                , getWidth() - 5, 40));
    }

    EffectPositioner::getInstance()->effectResized(this);
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

    if (selected) {
        strokeType.createStrokedPath(hoverRectangle, hoverRectangle);
    } else if (hovered) {
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(hoverRectangle, hoverRectangle, thiccness, 2);
    }


    if (selected || hovered) {
        g.strokePath(hoverRectangle, strokeType);
    }

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

bool Effect::hasPort(const ConnectionPort_old *port) {
    return (port->getParentComponent() == this);
}

bool Effect::hasConnection(const ConnectionLine *line) {
    return (hasPort(line->getOutPort())
        || hasPort(line->getInPort()));
}

Parameter *Effect::getParameterForPort(ParameterPort *port) {
    if (port->isInternal) {
        for (auto c : getChildren()) {
            if (auto param = dynamic_cast<Parameter *>(c)) {
                if (param->getPort(true) == port) {
                    return param;
                }
            }
        }
    } else {
        return dynamic_cast<Parameter*>(port->getParentComponent());
    }
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
Array<ConnectionPort_old *> Effect::getPorts(int isInput) {
    Array<ConnectionPort_old*> list;

    //todo add template type to cast

    for (auto c : getChildren()) {
        if (auto p = dynamic_cast<AudioPort*>(c)) {
            if (isInput == -1 || p->isInput == isInput) {
                list.add(p);
            }
        }
    }

    return list;
}

void Effect::mouseDown(const MouseEvent &event) {
    //fixme debug purposes
    for (auto port : getPorts(-1)) {
        if (port->isConnected()) {
            std::cout << "connected port" << newLine;
        } else {
            std::cout << "unconnected port" << newLine;
        }
    }

    //setAlwaysOnTop(true);
    if (dynamic_cast<Resizer*>(event.originalComponent)) {
        undoManager.beginNewTransaction("Resize");
        return;
    } else {
        if (event.mods.isRightButtonDown()) {

        }
        rightClickDragPos = getPosition();
        rightClickDragActivated = false;

        undoManager.beginNewTransaction("Move");
        dragger.startDraggingComponent(this, event);

        EffectBase::mouseDown(event);
    }
}

void Effect::mouseDrag(const MouseEvent &event) {
    if (event.originalComponent == this) {
        //resetHoverObject();

        SceneComponent::mouseDrag(event);

        dragger.dragComponent(this, event, nullptr);

        auto dragIntoObject = nullptr;//getDragIntoObject();

        /*if (auto newParent = dynamic_cast<EffectBase*>(dragIntoObject)) {
            if (newParent != getParentComponent()) {
                // Reassign parent
                EffectPositioner::getInstance()->effectParentReassigned(this, newParent);
            }
        } else {
            EffectPositioner::getInstance()->effectMoved(this);
        }*/
    } else {
        // Something else than the effect initiated the drag. Probably ConnectionPort_old.
        EffectBase::mouseDrag(event);
    }


}

void Effect::mouseUp(const MouseEvent &event) {
    //endDragHoverDetect();
    if (event.originalComponent == this) {
        SceneComponent::mouseUp(event);

        if (event.mods.isRightButtonDown() && event.getDistanceFromDragStart() < 10) {
            if (editMode) {
                callMenu(editMenu);
            } else {
                callMenu(menu);
            }
        }
        //getParentComponent()->mouseUp(event);
    } else {
        EffectBase::mouseUp(event);
    }
}

bool Effect::canDragInto(const SceneComponent& other, bool isRightClickDrag) const {
    auto effect = dynamic_cast<const Effect*>(&other);
    if (effect == nullptr) {
        return false;
    }

    return (effect->isInEditMode() && ! effect->isIndividual());
}

bool Effect::canDragHover(const SceneComponent &other, bool isRightClickDrag) const {
    if (! isRightClickDrag) {
        if (auto effect = dynamic_cast<const Effect*>(&other)) {
            return (effect->isInEditMode() && ! effect->isIndividual());
        }
        if (&other == this) {
            return false;
        }
        if (auto line = dynamic_cast<const ConnectionLine*>(&other)) {
            return ! (isParentOf(line->getInPort()) || isParentOf(line->getOutPort()));
        }
    }
    return false;
}

void Effect::addPort(AudioPort *port) {
    auto flexAudioPort = FlexItem(*port);
    flexAudioPort.height = port->getHeight();
    flexAudioPort.width = port->getWidth();
    flexAudioPort.margin = 10;
    flexAudioPort.alignSelf = FlexItem::AlignSelf::center;

    if (! isIndividual()) {
        auto flexInternalPort = FlexItem(*port->internalPort);

        flexInternalPort.width = port->getWidth();
        flexInternalPort.height = port->getHeight();
        flexInternalPort.margin = 10;
        flexInternalPort.alignSelf = FlexItem::AlignSelf::center;

        if (port->isInput) {
            internalInputPortFlexBox.items.add(flexInternalPort);
        } else {
            internalOutputPortFlexBox.items.add(flexInternalPort);
        }

        addChildComponent(*port->internalPort);
        port->internalPort->setVisible(isInEditMode());
    }

    if (port->isInput) {
        inputPortFlexBox.items.add(flexAudioPort);
    } else {
        outputPortFlexBox.items.add(flexAudioPort);
    }

    addAndMakeVisible(port);
}

/*void Effect::childrenChanged() {
//    resized();
    Component::childrenChanged();
}*/

void Effect::setName(const String &newName) {
    title.setText(newName, dontSendNotification);
    Component::setName(newName);
}


void Effect::removePort(ConnectionPort_old *port) { //todo test
    auto audioPort = dynamic_cast<AudioPort*> (port);
    if (audioPort == nullptr) {
        audioPort = dynamic_cast<AudioPort*>(port->getLinkedPort());
    }
    jassert(audioPort != nullptr);

    if (audioPort->isInput) {
        for (auto& item : internalInputPortFlexBox.items) {
            if (item.associatedComponent == audioPort->internalPort.get()) {
                internalInputPortFlexBox.items.remove(&item);
            }
        }
        for (auto& item : inputPortFlexBox.items) {
            if (item.associatedComponent == audioPort) {
                inputPortFlexBox.items.remove(&item);
            }
        }
    } else {
        for (auto& item : internalOutputPortFlexBox.items) {
            if (item.associatedComponent == audioPort->internalPort.get()) {
                //auto index = internalOutputPortFlexBox.items.indexOf(item);
                internalOutputPortFlexBox.items.remove(&item);
            }
        }
        for (auto& item : outputPortFlexBox.items) {
            if (item.associatedComponent == audioPort) {
                //auto index = outputPortFlexBox.items.indexOf(item);
                outputPortFlexBox.items.remove(&item);
            }
        }
    }

    removeChildComponent(audioPort);
    removeChildComponent(audioPort->internalPort.get());
}

void Effect::mouseDoubleClick(const MouseEvent &event) {
    setEditMode(! editMode);
    Component::mouseDoubleClick(event);
}

void Effect::childrenChanged() {
    /*for (auto c : getChildren()) {
        if (auto effect = dynamic_cast<Effect*>(c)) {
            if (! getLocalBounds().contains(effect->getBoundsInParent())) {
                expandToFitChildren();
            }
        }
    }*/
    
    if (appState != stopping && appState != loading) {
        resized();
        
        if (! isInEditMode()) {
            for (auto c : getChildren()) {
                if (dynamic_cast<Effect*>(c) || dynamic_cast<ConnectionLine*>(c)) {
                    c->setVisible(false);
                }
            }
        }
    }

    Component::childrenChanged();
}

void Effect::expandToFitChildren() {
    auto newBounds = getLocalBounds();

    for (auto c : getChildren()) {
        if (! newBounds.contains(c->getBoundsInParent())) {
            newBounds = newBounds.getUnion(c->getBoundsInParent());
        }
    }

    newBounds.expand(50, 80);

    newBounds.setPosition(getPosition());
    setBounds(newBounds);

    if (getParentComponent() != nullptr) {
        if (!getParentComponent()->getLocalBounds().contains(getBoundsInParent())) {
            if (auto effect = dynamic_cast<Effect *>(getParentComponent())) {
                effect->expandToFitChildren();
            }
        }
    }
}

Point<int> Effect::getPosWithinParent() {
    // Manual constraint
    auto newX = jlimit<int>(0, getParentWidth() - getWidth(), getX());
    auto newY = jlimit<int>(0, getParentHeight() - getHeight(), getY());

    return Point<int>(newX, newY);
}

void Effect::repositionInternals() {
    // Resize and position members

    //todo editmode: only set size if smaller than necessary

    auto newWidth = 250;
    auto newHeight = 250;
    auto parametersY = 0;

    paramPortsFlexBox.items.clear();
    parameterFlexBox.items.clear();

    parameterFlexBox.flexDirection = editMode ? FlexBox::Direction::row : FlexBox::Direction::column;

    for (auto parameter : getParameterChildren()) {
        FlexItem paramFlexItem;
        paramFlexItem.associatedComponent = parameter;
        paramFlexItem.width = parameter->getWidth();
        paramFlexItem.height = parameter->getHeight() + 15;

        paramFlexItem.alignSelf = FlexItem::AlignSelf::center;

        parameterFlexBox.items.add(paramFlexItem);

        parametersY = 0;
        for (auto c : getChildren()) {
            if (auto e = dynamic_cast<Effect *>(c)) {
                parametersY = jmax(parametersY, e->getBottom());
            }
        }
        parametersY += 20;

        FlexItem paramPortFlexItem(*parameter->getPort(true));
        paramPortFlexItem.width = 40;
        paramPortFlexItem.height = 40;
        paramPortFlexItem.margin = 0;
        paramPortFlexItem.alignSelf = FlexItem::AlignSelf::flexStart;

        paramPortsFlexBox.items.add(paramPortFlexItem);
    }

    if (editMode) {
        for (auto parameter : getParameterChildren()) {
            // Set internal port position
            auto port = parameter->getPort(true);
            port->setTopLeftPosition(parameter->getX() + getWidth() / 2, getHeight() - 40);
        }

        if (newHeight < parametersY + 100) {
            newHeight =  parametersY + 100;
        }

    } else {
        if (parameterFlexBox.items.size() > 0) {
            for (auto c : parameterFlexBox.items) {
                if (auto parameter = dynamic_cast<Parameter*>(c.associatedComponent)) {
                    if (dynamic_cast<ComboParameter*>(parameter) != nullptr) {
                        newWidth = jmax(newWidth, c.associatedComponent->getRight() + 30);
                    } else {
                        newWidth = jmax(newWidth, c.associatedComponent->getRight() + 10);
                    }
                }
            }

            newWidth += 40;
        }

        newWidth += 55;

        newHeight = parameterFlexBox.items.size() * 50 + 90;

        int numPortsHorizontally = (newWidth - 10) / 40;
        int numWraps = ceil(paramPortsFlexBox.items.size() / numPortsHorizontally);
        auto portsHeight = (60 + numWraps * 40);

        newHeight += numWraps * 40;
    }

    // Set size to necessary.
    if (newWidth > getWidth() || newHeight > getHeight()) {
        setSize(newWidth, newHeight);
    }
    resizer.setMinWidthAndHeight(newWidth, newHeight);
}

void Effect::updateConstrainerSize() {

}


bool ConnectionLine::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    return dynamic_cast<const Effect*>(other) != nullptr
        || dynamic_cast<const Parameter*>(other) != nullptr;
}

bool ConnectionLine::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    return dynamic_cast<const ConnectionPort_old*>(other);
}

bool ConnectionPort_old::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
}

bool ConnectionPort_old::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
}

