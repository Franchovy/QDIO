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
#include "Ports.h"


// Static members
EffectTreeBase::AppState EffectTreeBase::appState = neutral;
AudioProcessorGraph* EffectTreeBase::audioGraph = nullptr;
AudioProcessorPlayer* EffectTreeBase::processorPlayer = nullptr;
AudioDeviceManager* EffectTreeBase::deviceManager = nullptr;
UndoManager EffectTreeBase::undoManager;

const Identifier Effect::IDs::x = "x";
const Identifier Effect::IDs::y = "y";
const Identifier Effect::IDs::w = "w";
const Identifier Effect::IDs::h = "h";
const Identifier Effect::IDs::processorID = "processor";
const Identifier Effect::IDs::initialised = "initialised";
const Identifier Effect::IDs::name = "name";
const Identifier Effect::IDs::connections = "connections";
const Identifier Effect::IDs::EFFECT_ID = "effect";

EffectTreeBase* EffectTreeBase::effectScene = nullptr;


void EffectTreeBase::findLassoItemsInArea(Array<SelectHoverObject::Ptr> &results, const Rectangle<int> &area) {
    for (auto c : effectScene->getChildren()) {
        if (auto obj = dynamic_cast<SelectHoverObject*>(c)) {
            if (area.intersects(c->getBoundsInParent())) {
                results.addIfNotAlreadyThere(obj);
            }
        }
    }
}

SelectedItemSet<SelectHoverObject::Ptr>& EffectTreeBase::getLassoSelection() {
    deselectAll();
    return selected;
}


EffectTreeBase::EffectTreeBase() {
    //dragLine.setAlwaysOnTop(true);
}

bool EffectTreeBase::connectParameters(const ConnectionLine &connectionLine) {
    auto inPort = dynamic_cast<ParameterPort*>(connectionLine.getInPort().get());
    auto outPort = dynamic_cast<ParameterPort*>(connectionLine.getOutPort().get());

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
    return outParam->isConnected();
}

void EffectTreeBase::disconnectParameters(const ConnectionLine &connectionLine) {
    auto port = dynamic_cast<ParameterPort*>(connectionLine.getOutPort().get());
    jassert(port != nullptr);

    auto parameter = dynamic_cast<Parameter*>(dynamic_cast<Effect*>(
            port->getParentEffect())->getParameterForPort(port));

    if (parameter->isOutput()) {
        port = dynamic_cast<ParameterPort*>(connectionLine.getInPort().get());
        jassert(port != nullptr);

        parameter = dynamic_cast<Parameter*>(dynamic_cast<Effect*>(
                port->getParentEffect())->getParameterForPort(port));
    }

    jassert(parameter != nullptr);
    if (parameter->isConnected()) {
        parameter->disconnect(false);
    }
}


bool EffectTreeBase::connectAudio(const ConnectionLine& connectionLine) {
    auto connections = getAudioConnection(connectionLine);

    if (connections.isEmpty()) {
        return false;
    }

    auto connection = connections.getFirst();
    for (auto c = 0; c < 2; c++) { // default 2 channels
        connection.source.channelIndex = c;
        connection.destination.channelIndex = c;

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

    auto returnArray = Array<AudioProcessorGraph::Connection>();

    auto inPort = connectionLine.getOutPort();
    auto outPort = connectionLine.getInPort();

    if (inPort == nullptr || outPort == nullptr) {
        return returnArray;
    }

    auto inEVT = dynamic_cast<Effect *>(inPort->getParentComponent());
    auto outEVT = dynamic_cast<Effect *>(outPort->getParentComponent());

    if (inEVT == nullptr || outEVT == nullptr) {
        return returnArray;
    }

    in = inEVT->getNode(inPort);
    out = outEVT->getNode(outPort);


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

void EffectTreeBase::mouseDown(const MouseEvent &event) {
    //dynamic_cast<SelectHoverObject*>(event.originalComponent)->mouseDown(event);
    //SelectHoverObject::mouseDown(event);
    // Handle Connection
    if (auto port = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        dragLine = new ConnectionLine();

        dragLine->setPort(port);

        auto parent = port->getDragLineParent();

        parent->addAndMakeVisible(dragLine);
        parent->addMouseListener(dragLine, true);
        //dragLine->mouseDown(event);
    }
}

void EffectTreeBase::mouseDrag(const MouseEvent &event) {
    //dynamic_cast<SelectHoverObject*>(event.originalComponent)->mouseDrag(event);
    //SelectHoverObject::mouseDrag(event);
    // Handle Connection
    /*if (auto port = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        //dragLine->mouseDrag(event);

    }*/
}

void EffectTreeBase::mouseUp(const MouseEvent &event) {
    //dynamic_cast<SelectHoverObject*>(event.originalComponent)->mouseUp(event);
    //SelectHoverObject::mouseUp(event);
    // Handle Connection
    //dragLine->mouseUp(event);
    if (dragLine != nullptr) {
        // Connection made?
        auto obj = getDragIntoObject();
        if (auto port = dynamic_cast<ConnectionPort*>(obj)) {
            //dragLine->setPort(port);
        } else {
            //connections.removeObject(dragLine);
        }

    }
}

Array<ConnectionLine *> EffectTreeBase::getConnectionsToThis(bool isInputConnection) {
    Array<ConnectionLine*> array;
    for (auto c : getParentComponent()->getChildren()) {
        if (auto line = dynamic_cast<ConnectionLine*>(c)) {
            if (isInputConnection && isParentOf(line->getInPort().get())) {
                array.add(line);
            } else if (! isInputConnection && isParentOf(line->getOutPort().get())) {
                array.add(line);
            }
        }
    }

    return array;
}

Array<ConnectionLine *> EffectTreeBase::getConnectionsToThis() {
    Array<ConnectionLine*> array;
    for (auto c : getParentComponent()->getChildren()) {
        if (auto line = dynamic_cast<ConnectionLine*>(c)) {
            if (isParentOf(line->getInPort().get()) || isParentOf(line->getOutPort().get())) {
                array.add(line);
            }
        }
    }

    return array;
}


ConnectionPort* EffectTreeBase::getPortFromID(String portID) {
    auto ref = Identifier(portID);

    if (auto p = findChildWithID(ref)) {
        return dynamic_cast<ConnectionPort*>(p);
    }

    for (auto child : getChildren()) {
        if (auto effect = dynamic_cast<Effect*>(child)) {
            for (auto p : effect->getParameterChildren()) {
                if (auto port = p->getPortWithID(portID)) {
                    return port;
                }
            }

            if (auto p = child->findChildWithID(ref)) {
                return dynamic_cast<ConnectionPort*>(p);
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

void EffectTreeBase::handleCommandMessage(int commandId) {
    if (commandId == 9) {
        undoManager.beginNewTransaction();
    }
    Component::handleCommandMessage(commandId);
}

Array<ConnectionLine *> EffectTreeBase::getConnectionsInside() {
    Array<ConnectionLine *> array;
    for (auto c : getChildren()) {
        if (auto line = dynamic_cast<ConnectionLine*>(c)) {
            array.add(line);
        }
    }
    return array;
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

    internalInputPortFlexBox.flexDirection = internalOutputPortFlexBox.flexDirection =
            outputPortFlexBox.flexDirection = inputPortFlexBox.flexDirection = FlexBox::Direction::column;
    internalInputPortFlexBox.justifyContent = internalOutputPortFlexBox.justifyContent =
        outputPortFlexBox.justifyContent = inputPortFlexBox.justifyContent = FlexBox::JustifyContent::center;

    title.setBounds(30,30,200, title.getFont().getHeight());
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
        addPort(getDefaultBus(), true);
        resized();
    });
    portSubMenu->addItem("Output Port", [=](){
        addPort(getDefaultBus(), false);
        resized();
    });
    portSubMenuItem.subMenu = std::move(portSubMenu);

    parameterSubMenu->addItem("Slider", [=] () {
        undoManager.beginNewTransaction("Add slider parameter");

        auto parameter = new Parameter(nullptr, 2, true);
        addAndMakeVisible(parameter);
        //parameter->setCentrePosition(getMouseXYRelative());
    });
    parameterSubMenu->addItem("Selection Box", [=] () {
        undoManager.beginNewTransaction("Add combo parameter");

        auto parameter = new Parameter(nullptr, 1, true);
        addAndMakeVisible(parameter);
        //parameter->setCentrePosition(getMouseXYRelative());
    });
    parameterSubMenu->addItem("Toggle Button", [=] () {
        undoManager.beginNewTransaction("Add button parameter");

        auto parameter = new Parameter(nullptr, 0, true);
        addAndMakeVisible(parameter);
        //parameter->setCentrePosition(getMouseXYRelative());
    });
    addParamSubMenuItem.subMenu = std::move(parameterSubMenu);

    addMenuItem(editMenu, toggleEditMode);
    addMenuItem(menu, toggleEditMode);
    addMenuItem(editMenu, changeEffectImage);
    addMenuItem(editMenu, portSubMenuItem);
    addMenuItem(editMenu, addParamSubMenuItem);

}

Effect::~Effect()
{
    //audioGraph->removeAllChangeListeners();

    for (auto p : parameterArray) {
        // Remove listeners
        p->removeListeners();
        removeChildComponent(p);
        removeChildComponent(p->getPort(true));
    }
}

// Processor hasEditor? What to do if processor is a predefined plugin
void Effect::setProcessor(AudioProcessor *processor) {
    this->processor = processor;

    // Processor settings (how best to do this?)
    node->getProcessor()->setPlayConfigDetails(
            processor->getTotalNumInputChannels(),
            processor->getTotalNumOutputChannels(),
            audioGraph->getSampleRate(),
            audioGraph->getBlockSize());

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
            c->setInterceptsMouseClicks(false, false);
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
                p->setInterceptsMouseClicks(false, true);
            }
        }

        title.setMouseCursor(getMouseCursor());
        title.setInterceptsMouseClicks(false,false);
        title.setColour(title.textColourId, Colours::black);
    }

    resized();
    repaint();
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
        auto linkedPort = port->getLinkedPort();
        if (linkedPort->isConnected()) {
            auto otherPort = linkedPort->getOtherPort();
            nodeAndPort = dynamic_cast<Effect*>(otherPort->getParentComponent())->getNode(
                    otherPort);
            return nodeAndPort;
        } else return nodeAndPort;
    }
}

AudioProcessorGraph::NodeID Effect::getNodeID() const {
    return node->nodeID;
}


void Effect::resized() {
    inputPortFlexBox.performLayout(Rectangle<int>(10, 18, 60, getHeight() - 18));
    outputPortFlexBox.performLayout(Rectangle<int>(getWidth() - 90, 18, 60, getHeight() - 18));
    internalInputPortFlexBox.performLayout(Rectangle<int>(80, 30, 60, getHeight()));
    internalOutputPortFlexBox.performLayout(Rectangle<int>(getWidth() - 120, 30, 60, getHeight()));

    if (! editMode) {

        // Parameters
        FlexBox parameterFlexBox;
        FlexBox paramPortsFlexBox;

        for (auto parameter : getParameterChildren()) {
            FlexItem paramFlexItem;
            paramFlexItem.associatedComponent = parameter;
            if (parameter->type == Parameter::slider) {
                paramFlexItem.width = 150;
                paramFlexItem.height = 120;
            } else if (parameter->type == Parameter::combo) {
                paramFlexItem.width = 150;
                paramFlexItem.height = 110;
            } else if (parameter->type == Parameter::button) {
                paramFlexItem.width = 120;
                paramFlexItem.height = 110;
            }
            paramFlexItem.alignSelf = FlexItem::AlignSelf::flexStart;

            parameterFlexBox.items.add(paramFlexItem);

            FlexItem paramPortFlexItem(*parameter->getPort(true));
            paramPortFlexItem.width = 60;
            paramPortFlexItem.height = 60;
            paramPortFlexItem.margin = 5;
            paramPortFlexItem.alignSelf = FlexItem::AlignSelf::flexStart;

            paramPortsFlexBox.items.add(paramPortFlexItem);
        }

        parameterFlexBox.flexDirection = FlexBox::Direction::column;
        parameterFlexBox.alignContent = FlexBox::AlignContent::flexStart;
        parameterFlexBox.alignItems = FlexBox::AlignItems::center;
        parameterFlexBox.justifyContent = FlexBox::JustifyContent::center;

        paramPortsFlexBox.flexDirection = FlexBox::Direction::row;
        paramPortsFlexBox.alignItems = FlexBox::AlignItems::flexStart;
        paramPortsFlexBox.alignContent = FlexBox::AlignContent::flexStart;
        paramPortsFlexBox.justifyContent = FlexBox::JustifyContent::flexStart;


        // Resize

        auto newWidth = 250;
        auto newHeight = 200;

        /*if (parameterFlexBox.items.size() > 0) {
            *//*for (auto c : parameterFlexBox.items) {
                int widthToAdd;
                if (auto parameter = dynamic_cast<Parameter*>(c.associatedComponent)) {
                    if (parameter->type == Parameter::combo) {
                        newWidth = jmax(newWidth, c.associatedComponent->getRight() + 40);
                    } else {
                        newWidth = jmax(newWidth, c.associatedComponent->getRight() + 10);
                    }
                }
            }*//*

            newWidth += 40;
            newHeight = parameterFlexBox.items.size() * 50 + 90;
        }
        if (outputPortFlexBox.items.size() > 0) {
            //newWidth += 20;
        }*/
        newHeight = parameterFlexBox.items.size() * 50 + 90;

        setSize(newWidth, newHeight);

        // Perform layout

        parameterFlexBox.performLayout(Rectangle<int>((inputPortFlexBox.items.size() > 0) ? 60 : 15
                , 50, (outputPortFlexBox.items.size() > 0) ? getWidth() - 60 : getWidth() - 15
                , getHeight() - 90));
        paramPortsFlexBox.performLayout(Rectangle<int>(20, getHeight() - 60
                , getWidth() - 20, 60));
    } else {
        if (getParameterChildren().size() > 0) {

            FlexBox parameterFlexBox;

            auto parametersY = 0;
            for (auto c : getChildren()) {
                if (auto e = dynamic_cast<Effect *>(c)) {
                    parametersY = jmax(parametersY, e->getBottom());
                }
            }
            parametersY += 20;

            for (auto parameter : getParameterChildren()) {

                FlexItem parameterFlexItem(*parameter);
                if (parameter->type == Parameter::slider) {
                    parameterFlexItem.width = 150;
                    parameterFlexItem.height = 120;
                } else if (parameter->type == Parameter::combo) {
                    parameterFlexItem.width = 150;
                    parameterFlexItem.height = 110;
                } else if (parameter->type == Parameter::button) {
                    parameterFlexItem.width = 120;
                    parameterFlexItem.height = 110;
                }
                parameterFlexBox.items.add(parameterFlexItem);

                /*// Set internal port position
                auto port = parameter->getPort(true);
                port->setTopLeftPosition(parameter->getX() + getWidth() / 2, getHeight() - 40);*/
            }

            parameterFlexBox.flexDirection = FlexBox::Direction::row;
            parameterFlexBox.flexWrap = FlexBox::Wrap::noWrap;
            parameterFlexBox.justifyContent = FlexBox::JustifyContent::center;
            parameterFlexBox.alignContent = FlexBox::AlignContent::center;
            parameterFlexBox.alignItems = FlexBox::AlignItems::center;

            if (getHeight() < parametersY + 100) {
                setSize(getWidth(), parametersY + 100);
            }

            parameterFlexBox.performLayout(Rectangle<int>(20, parametersY, getWidth() - 40, 100));
        }
    }
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

    if (selectMode) {
        strokeType.createStrokedPath(hoverRectangle, hoverRectangle);
    } else if (hoverMode) {
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(hoverRectangle, hoverRectangle, thiccness, 2);
    }


    if (selectMode || hoverMode) {
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

bool Effect::hasPort(const ConnectionPort *port) {
    return (port->getParentComponent() == this);
}

bool Effect::hasConnection(const ConnectionLine *line) {
    return (hasPort(line->getOutPort().get())
        || hasPort(line->getInPort().get()));
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
Array<ConnectionPort *> Effect::getPorts(int isInput) {
    Array<ConnectionPort*> list;

    //todo add template type to cast

    for (auto c : getChildren()) {
        if (auto p = dynamic_cast<AudioPort*>(c)) {
            if (p->isInput == isInput) {
                list.add(p);
            }
        }
    }

    return list;
}

void Effect::mouseDown(const MouseEvent &event) {
    //setAlwaysOnTop(true);
    if (dynamic_cast<Resizer*>(event.originalComponent)) {
        undoManager.beginNewTransaction("Resize");
        return;
    } else {
        if (event.mods.isRightButtonDown()) {
            rightClickDragPos = getPosition();
            rightClickDragActivated = false;
        }

        undoManager.beginNewTransaction("Move");
        dragger.startDraggingComponent(this, event);
        startDragHoverDetect();

        EffectTreeBase::mouseDown(event);
    }
}

void Effect::mouseDrag(const MouseEvent &event) {
    if (event.originalComponent == this) {
        SelectHoverObject::mouseDrag(event);

        dragger.dragComponent(this, event, nullptr);

        setTopLeftPosition(getPosWithinParent());

        auto dragIntoObject = getDragIntoObject();

        if (event.mods.isLeftButtonDown()) {
            resetHoverObject();

            if (dragIntoObject != nullptr && dragIntoObject != getParentComponent()) {
                std::cout << "Drag into: " << dragIntoObject->getName() << newLine;
                if (auto newParent = dynamic_cast<EffectTreeBase *>(dragIntoObject)) {
                    auto oldParent = dynamic_cast<EffectTreeBase *>(getParentComponent());
                    auto connections = getConnectionsToThis();
                    ConnectionLine *outgoingConnection = nullptr;

                    // Reassign effect parent
                    /**/
                    newParent->addAndMakeVisible(this);

                    if (getParentHeight() - getHeight() < 0 || getParentWidth() - getWidth() < 0) {
                        // Expand child to fit
                        auto newPos = newParent->getBoundsInParent().getConstrainedPoint(getPosition());
                        newPos = newParent->getLocalPoint(newParent->getParentComponent(), newPos);
                        setTopLeftPosition(newPos);
                        auto parentEffect = dynamic_cast<Effect*>(newParent);
                        jassert(parentEffect != nullptr);
                        parentEffect->expandToFitChildren();
                        setTopLeftPosition(getPosWithinParent());
                    } else {
                        // Position effect inside parent
                        setTopLeftPosition(getPosWithinParent());
                    }

                    //auto outParent = (newParent->isParentOf(oldParent)) ? newParent : oldParent;
                    auto parent = (newParent->isParentOf(oldParent)) ? oldParent : newParent;

                    // Adjust connections accordingly
                    for (auto c : connections) {
                        if (dynamic_cast<ParameterPort*>(c->getInPort().get())) {
                            // Remove parameter connection
                            c->getParentComponent()->removeChildComponent(c);
                        } else {
                            // Check if line is connected to an internal port
                            if (c->getInPort().get()->getParentComponent() == parent
                                || c->getOutPort().get()->getParentComponent() == parent) {
                                auto connectionsToShorten = (newParent->isParentOf(oldParent))
                                                            ? parent->getConnectionsToThis()
                                                            : parent->getConnectionsInside();

                                for (auto c_out : connectionsToShorten) {
                                    if (c_out->getOutPort() == c->getInPort()->getLinkedPort()
                                        || c_out->getInPort() == c->getOutPort()->getLinkedPort()) {
                                        outgoingConnection = c_out;
                                    }
                                }

                                if (outgoingConnection != nullptr) {
                                    if (newParent->isParentOf(oldParent)) {
                                        shortenConnection(c, outgoingConnection);
                                    } else {
                                        shortenConnection(outgoingConnection, c);
                                    }
                                } else {
                                    parent->removeChildComponent(c);
                                }
                            } else {
                                // Connection to extend
                                extendConnection(c, dynamic_cast<Effect *>(parent));
                            }
                        }
                    }
                }
            }
        } else if (event.mods.isRightButtonDown()) {
            if (! rightClickDragActivated
                    && (getPosition().getDistanceFrom(rightClickDragPos) > 50)) {

                rightClickDragActivated = true;

                auto ingoingConnections = getConnectionsToThis(true);
                auto outgoingConnections = getConnectionsToThis(false);
                if (ingoingConnections.size() != 0 || outgoingConnections.size() != 0) {
                    // Disconnect from any connections
                    if (ingoingConnections.size() == outgoingConnections.size()) {
                        mergeConnection(ingoingConnections.getFirst(), outgoingConnections.getFirst());
                    } else if (ingoingConnections.size() == 0 && outgoingConnections.size() > 1) {
                        getParentComponent()->removeChildComponent(outgoingConnections.getFirst());
                    } else if (outgoingConnections.size() == 0 && ingoingConnections.size() > 1) {
                        getParentComponent()->removeChildComponent(ingoingConnections.getFirst());
                    }
                }
            }

            if (dragIntoObject != nullptr) {
                if (auto lineToJoin = dynamic_cast<ConnectionLine*>(dragIntoObject)) {
                    // Join this connection
                    auto inPorts = getPorts(true);
                    auto outPorts = getPorts(false);
                    // Add port if we want to connect this up
                    if (!isIndividual()
                        && (inPorts.size() == 0 || outPorts.size() == 0)) {
                        if (inPorts.size() == 0) {
                            auto newPort = addPort(getDefaultBus(), true);
                            inPorts.add(newPort.get());
                        }
                        if (outPorts.size() == 0) {
                            auto newPort = addPort(getDefaultBus(), false);
                            outPorts.add(newPort.get());
                        }
                        resized();
                    }
                    // Replace single port with this one
                    if (inPorts.size() == 0) {
                        lineToJoin->unsetPort(lineToJoin->getOutPort().get());
                        lineToJoin->setPort(outPorts.getFirst());
                        rightClickDragPos = getPosition();
                        rightClickDragActivated = false;
                    }
                    if (outPorts.size() == 0) {
                        lineToJoin->unsetPort(lineToJoin->getInPort().get());
                        lineToJoin->setPort(inPorts.getFirst());
                        rightClickDragPos = getPosition();
                        rightClickDragActivated = false;
                    }
                    if (inPorts.size() > 0 && outPorts.size() > 0) {
                        lineToJoin->reconnect(inPorts.getFirst(), outPorts.getFirst());
                        rightClickDragPos = getPosition();
                        rightClickDragActivated = false;
                    }
                }
            }
        }

        getParentComponent()->mouseDrag(event);
    } else {
        EffectTreeBase::mouseDrag(event);
    }
}

void Effect::mouseUp(const MouseEvent &event) {
    endDragHoverDetect();
    if (event.originalComponent == this) {
        SelectHoverObject::mouseUp(event);

        if (event.mods.isRightButtonDown() && event.getDistanceFromDragStart() < 10) {
            if (editMode) {
                callMenu(editMenu);
            } else {
                callMenu(menu);
            }
        }
        //getParentComponent()->mouseUp(event);
    } else {
        EffectTreeBase::mouseUp(event);
    }
}

bool Effect::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    auto effect = dynamic_cast<const Effect*>(other);
    if (effect == nullptr) {
        return false;
    }

    return (effect->isInEditMode() && ! effect->isIndividual());
}


bool Effect::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    if (! isRightClickDrag) {
        if (auto effect = dynamic_cast<const Effect*>(other)) {
            return (effect->isInEditMode() && ! effect->isIndividual());
        }
    } else {
        if (other == this) {
            return false;
        }
        if (auto line = dynamic_cast<const ConnectionLine*>(other)) {
            return ! (isParentOf(line->getInPort().get()) || isParentOf(line->getOutPort().get()));
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

void Effect::mergeConnection(ConnectionLine *inLine, ConnectionLine *outLine) {
    jassert(getPorts(true).contains(inLine->getInPort().get()));
    jassert(getPorts(false).contains(outLine->getOutPort().get()));

    auto newInPort = outLine->getInPort().get();
    outLine->getParentComponent()->removeChildComponent(outLine);

    inLine->unsetPort(inLine->getInPort().get());
    inLine->setPort(newInPort);
}

void Effect::extendConnection(ConnectionLine *lineToExtend, Effect *parentToExtendThrough) {
    if (lineToExtend->getInPort()->getDragLineParent() == parentToExtendThrough
            || lineToExtend->getOutPort()->getDragLineParent() == parentToExtendThrough)
    {
        // Exit parent
        bool isInput = parentToExtendThrough->isParentOf(lineToExtend->getInPort().get());

        auto newPort = parentToExtendThrough->addPort(getDefaultBus(), isInput);
        auto oldPort = isInput ? lineToExtend->getInPort().get() : lineToExtend->getOutPort().get();

        // Set line ports
        lineToExtend->unsetPort(oldPort);
    parentToExtendThrough->getParentComponent()->addAndMakeVisible(lineToExtend);
        lineToExtend->setPort(newPort.get());

        // Create new internal line
        auto newConnection = new ConnectionLine();
        parentToExtendThrough->addAndMakeVisible(newConnection);
        parentToExtendThrough->resized();
        newConnection->setPort(oldPort);
        newConnection->setPort(newPort->internalPort.get());
    } else {
        // Enter parent
        auto isInput = lineToExtend->getInPort()->getDragLineParent() == parentToExtendThrough;
        auto oldPort = isInput ? lineToExtend->getInPort().get() : lineToExtend->getOutPort().get();

        auto newPort = addPort(getDefaultBus(), isInput);
        lineToExtend->unsetPort(oldPort);
        lineToExtend->setPort(newPort.get());

        auto newConnection = new ConnectionLine();
        parentToExtendThrough->addAndMakeVisible(newConnection);
        newConnection->setPort(oldPort);
        newConnection->setPort(newPort->internalPort.get());
    }
}

void Effect::shortenConnection(ConnectionLine *interiorLine, ConnectionLine *exteriorLine) {
    auto parent = dynamic_cast<EffectTreeBase*>(exteriorLine->getParentComponent());
    auto targetEffect = dynamic_cast<Effect*>(interiorLine->getParentComponent());

    jassert(targetEffect->getParentComponent() == parent);

    // Check which port has moved
    if (interiorLine->getInPort()->getDragLineParent() != targetEffect
        || interiorLine->getOutPort()->getDragLineParent() != targetEffect)
    {
        // interior line to remove
        auto portToRemove = interiorLine->getInPort()->getParentComponent() == targetEffect
                ? interiorLine->getInPort().get() : interiorLine->getOutPort().get();
        auto newPort = interiorLine->getInPort()->getParentComponent() == targetEffect
                           ? interiorLine->getOutPort().get() : interiorLine->getInPort().get();
        targetEffect->removeChildComponent(interiorLine);

        exteriorLine->unsetPort(portToRemove->getLinkedPort());
        exteriorLine->setPort(newPort);

        // Remove unused port
        targetEffect->removePort(portToRemove);
    } else {
        // exterior line to remove
        auto portToRemove = exteriorLine->getInPort()->getParentComponent() == targetEffect
                            ? exteriorLine->getInPort().get() : exteriorLine->getOutPort().get();
        auto portToReconnect = exteriorLine->getInPort()->getParentComponent() == targetEffect
                       ? exteriorLine->getOutPort().get() : exteriorLine->getInPort().get();
        targetEffect->removeChildComponent(exteriorLine);

        interiorLine->unsetPort(portToRemove->getLinkedPort());
        interiorLine->setPort(portToReconnect);

        // Remove line
        parent->removeChildComponent(exteriorLine);

        // Remove unused port
        targetEffect->removePort(portToRemove);
    }
}

void Effect::removePort(ConnectionPort *port) { //todo test
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
    
    if (appState != stopping) {
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


bool ConnectionLine::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    return dynamic_cast<const Effect*>(other) != nullptr
        || dynamic_cast<const Parameter*>(other) != nullptr;
}

bool ConnectionLine::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    return dynamic_cast<const ConnectionPort*>(other);
}

bool ConnectionPort::canDragHover(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
}

bool ConnectionPort::canDragInto(const SelectHoverObject *other, bool isRightClickDrag) const {
    return false;
}

