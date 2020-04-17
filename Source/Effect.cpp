/*
  ==============================================================================

    Effect.cpp
    Created: 31 Mar 2020 9:03:16pm
    Author:  maxime

  ==============================================================================
*/

#include "Effect.h"
#include "IDs"

// Static members
AudioProcessorGraph EffectTreeBase::audioGraph;
AudioProcessorPlayer EffectTreeBase::processorPlayer;
AudioDeviceManager EffectTreeBase::deviceManager;
UndoManager EffectTreeBase::undoManager;
LineComponent EffectTreeBase::dragLine;

ReferenceCountedArray<Effect> EffectTreeBase::effectsToDelete;


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
            if (childTree.hasType(Effect::IDs::EFFECT_ID)) {
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
    }
    return nullptr;
}
//TODO
ConnectionPort::Ptr EffectTreeBase::portToConnectTo(const MouseEvent& event, const ValueTree& effectTree) {

    // Check for self ports
    if (auto p = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        if (auto e = getFromTree<Effect>(effectTree)) {
            if (auto returnPort = e->checkPort(e->getLocalPoint(event.eventComponent, event.getPosition()))) {
                if (p->canConnect(returnPort)) {
                    return returnPort;
                }
            }
        }
    }

    // Check children for a match
    for (int i = 0; i < effectTree.getNumChildren(); i++) {
        auto e = getFromTree<Effect>(effectTree.getChild(i));

        if (e == nullptr)
            continue;

        // Filter self effect if AudioPort
        if (auto p = dynamic_cast<AudioPort*>(event.originalComponent))
            if (p->getParentComponent() == e)
                continue;

        auto localPos = e->getLocalPoint(event.eventComponent, event.getPosition());

        if (e->contains(localPos))
        {
            if (auto p = e->checkPort(localPos))
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

bool EffectTreeBase::connectAudio(const ConnectionLine& connectionLine) {
    for (auto connection : getAudioConnection(connectionLine)) {
        if (!EffectTreeBase::audioGraph.isConnected(connection) &&
            EffectTreeBase::audioGraph.isConnectionLegal(connection)) {
            // Make audio connection
            return EffectTreeBase::audioGraph.addConnection(connection);
        }
    }
}

void EffectTreeBase::disconnectAudio(const ConnectionLine &connectionLine) {
    for (auto connection : getAudioConnection(connectionLine)) {
        if (audioGraph.isConnected(connection)) {
            audioGraph.removeConnection(connection);
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
        for (int c = 0; c < jmin(EffectTreeBase::audioGraph.getTotalNumInputChannels(),
                                 EffectTreeBase::audioGraph.getTotalNumOutputChannels()); c++) {
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
    for (auto e : effectsToDelete) {
        e->getTree().removeAllProperties(nullptr);
    }

    SelectHoverObject::close();

    effectsToDelete.clear();

    processorPlayer.setProcessor(nullptr);
    deviceManager.closeAudioDevice();
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

void EffectTreeBase::createConnection(ConnectionLine::Ptr line) {
    // Add connection to this object


}

PopupMenu EffectTreeBase::getEffectSelectMenu() {
    createEffectMenu.addItem("Empty Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("New Empty Effect");
                ValueTree newEffect(Effect::IDs::EFFECT_ID);

                newEffect.setProperty(Effect::IDs::x, getMouseXYRelative().x, nullptr);
                newEffect.setProperty(Effect::IDs::y, getMouseXYRelative().y, nullptr);

                //newEffect.setProperty(Effect::IDs::pos, Position::toVar(getMouseXYRelative()), nullptr);
                this->getTree().appendChild(newEffect, &undoManager);

                if (selected.getNumSelected() > 0) {
                    for (auto s : selected.getItemArray()) {
                        if (auto e = dynamic_cast<Effect*>(s.get())){
                            newEffect.appendChild(e->getTree(), &undoManager);
                        }
                    }
                }
            }));
    createEffectMenu.addItem("Input Device", std::function<void()>(
            [=]{
                newEffect("Input Device", 0);
            }));
    createEffectMenu.addItem("Output Device", std::function<void()>(
            [=]{
                newEffect("Output Device", 1);
            }));
    createEffectMenu.addItem("Delay Effect", std::function<void()>(
            [=](){
                newEffect("Delay Effect", 3);
            }
    ));
    createEffectMenu.addItem("Distortion Effect", std::function<void()>(
            [=]{
                newEffect("Distortion Effect", 2);
            }
    ));

    return createEffectMenu;
}

void EffectTreeBase::newEffect(String name, int processorID) {
    undoManager.beginNewTransaction("New " + name);
    ValueTree newEffect(Effect::IDs::EFFECT_ID);
    newEffect.setProperty(Effect::IDs::x, getMouseXYRelative().x, nullptr);
    newEffect.setProperty(Effect::IDs::y, getMouseXYRelative().y, nullptr);

    //newEffect.setProperty(Effect::IDs::pos, Position::toVar(getMouseXYRelative()), nullptr);
    newEffect.setProperty(Effect::IDs::processorID, processorID, nullptr);

    this->getTree().appendChild(newEffect, &undoManager);
}

/*void EffectTreeBase::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    *//*if (tree.hasType(CONNECTION_ID))
        std::cout << "Type connectionLine" << newLine;

    if (property == IDs::connections) {


        for (auto connection : tree) {
            // Connections removed from activeConnections
            if (connection->isVisible() && ! activeConnections.contains(ConnectionVar::fromVar(connection))) {
                std::cout << "Remove connection" << newLine;
                // Remove from audio
                disconnectAudio(*connection);

                connection->setVisible(false);
            }
            // Connections added to activeConnections
            else if (! connection->isVisible() && activeConnections.contains(ConnectionVar::fromVar(connection))) {
                std::cout << "Add connection" << newLine;
                // Add to audio
                connectAudio(*connection);

                connection->setVisible(true);
            } else {
                std::cout << "Schtruderloooder" << newLine;
            }
        }


        // Depending on if connection is visible or not, make or remove the audio connections.
        *//**//*for (auto connection : connections) {
            // Get connections to make or remove
            auto audioConnections = getAudioConnection(*connection);
            for (auto audioConnection : audioConnections) {
                // If connection is visible it should be created
                if (connection->isVisible() && ! audioGraph.isConnected(audioConnection)) {
                    if (audioGraph.isConnectionLegal(audioConnection)) {
                        audioGraph.addConnection(audioConnection);
                    }
                }
                // If connection is not visible it should be removed
                else if (! connection->isVisible() && audioGraph.isConnected(audioConnection)) {
                    audioGraph.removeConnection(audioConnection);
                }
            }
        }*//**//*
    }
*//*
    //Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}*/


void EffectTreeBase::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    std::cout << "VT child added" << newLine;
    // if effect has been created already
    if (childWhichHasBeenAdded.hasType(EFFECT_ID)) {
        if (childWhichHasBeenAdded.hasProperty(Effect::IDs::initialised)) {
            if (auto e = getFromTree<Effect>(childWhichHasBeenAdded)) {
                e->setVisible(true);
                // Adjust pos
                if (auto parent = getFromTree<EffectTreeBase>(parentTree)) {
                    if (!parent->getChildren().contains(e)) {
                        parent->addAndMakeVisible(e);
                        std::cout << "Pre pos: " << e->getPosition().toString() << newLine;
                        e->setTopLeftPosition(parent->getLocalPoint(e, e->getPosition()));
                        std::cout << "Post pos: " << e->getPosition().toString() << newLine;
                    }
                }
            }
        } else {
            // Initialise VT
            childWhichHasBeenAdded.setProperty(Effect::IDs::initialised, true, &undoManager);
            // Create new effect
            auto e = new Effect(childWhichHasBeenAdded);
            childWhichHasBeenAdded.setProperty(IDs::effectTreeBase, e, nullptr);
        }
    } else if (childWhichHasBeenAdded.hasType(CONNECTION_ID)) {
        ConnectionLine* line;
        // Add connection here
        if (! childWhichHasBeenAdded.hasProperty(ConnectionLine::IDs::ConnectionLineObject)) {
            auto inport = getPropertyFromTree<ConnectionPort>(childWhichHasBeenAdded, ConnectionLine::IDs::InPort);
            auto outport = getPropertyFromTree<ConnectionPort>(childWhichHasBeenAdded, ConnectionLine::IDs::OutPort);


            line = new ConnectionLine(*inport, *outport);
            childWhichHasBeenAdded.setProperty(ConnectionLine::IDs::ConnectionLineObject, line, nullptr);
            addAndMakeVisible(line);
        } else {
            line = getPropertyFromTree<ConnectionLine>(childWhichHasBeenAdded, ConnectionLine::IDs::ConnectionLineObject);
            line->setVisible(true);
        }

        connectAudio(*line);
    }
}

void EffectTreeBase::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                           int indexFromWhichChildWasRemoved) {
    std::cout << "VT child removed" << newLine;
    if (childWhichHasBeenRemoved.hasType(EFFECT_ID)) {
        if (auto e = getFromTree<Effect>(childWhichHasBeenRemoved)) {
            if (auto parent = getFromTree<EffectTreeBase>(parentTree)) {

                parent->removeChildComponent(e);
                e->setVisible(false);
            }
        }
    } else if (childWhichHasBeenRemoved.hasType(CONNECTION_ID)) {
        auto line = getPropertyFromTree<ConnectionLine>(childWhichHasBeenRemoved, ConnectionLine::IDs::ConnectionLineObject);
        line->setVisible(false);
        disconnectAudio(*line);
    }
}

template<class T>
T *EffectTreeBase::getFromTree(const ValueTree &vt) {
    return dynamic_cast<T*>(vt.getProperty(IDs::effectTreeBase).getObject());
}

template<class T>
T *EffectTreeBase::getPropertyFromTree(const ValueTree &vt, Identifier property) {
    return dynamic_cast<T*>(vt.getProperty(property).getObject());
}

bool EffectTreeBase::keyPressed(const KeyPress &key) {
    if (key.getKeyCode() == 's') {
        std::cout << "Audiograph status: " << newLine;
        for (auto node : audioGraph.getNodes()) {
            if (node != nullptr) {
                std::cout << "node: " << node->nodeID.uid << newLine;
                std::cout << node->getProcessor()->getName() << newLine;
            } else {
                std::cout << "Null node." << newLine;
            }
        }
    }
    if (key.getKeyCode() == KeyPress::deleteKey || key.getKeyCode() == KeyPress::backspaceKey) {
        for (const auto& selectedItem : selected.getItemArray()) {
            if (auto l = dynamic_cast<ConnectionLine*>(selectedItem.get())) {
                auto lineTree = tree.getChildWithProperty(ConnectionLine::IDs::ConnectionLineObject, l);
                tree.removeChild(lineTree, &undoManager);
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
    /*if (getReferenceCount() > 0) {
        std::cout << "Warning: Reference count greater than zero. Exceptions may occur" << newLine;
        resetReferenceCount();
    }*/
}



void EffectTreeBase::callMenu(PopupMenu& m) {
    // Execute result
    int result = m.show();

}

void EffectTreeBase::mouseDown(const MouseEvent &event) {
    std::cout << "Begin new transaction" << newLine;
    undoManager.beginNewTransaction(getName());

}

void EffectTreeBase::mouseDrag(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.dragLasso(event);

    if (event.mods.isLeftButtonDown()) {
        // Line drag
        if (dynamic_cast<ConnectionPort*>(event.originalComponent)) {
            auto port = portToConnectTo(event, tree);

            if (port != nullptr) {
                SelectHoverObject::setHoverComponent(port);
            } else {
                SelectHoverObject::resetHoverObject();
            }
        }
        /*
        if (dynamic_cast<LineComponent *>(event.eventComponent)) {
            // ParentToCheck is the container of possible things to connect to.
            Component *parentToCheck;
            if (dynamic_cast<AudioPort *>(event.originalComponent))
                parentToCheck = event.originalComponent->getParentComponent()->getParentComponent();
            else if (dynamic_cast<InternalConnectionPort *>(event.originalComponent))
                parentToCheck = event.originalComponent->getParentComponent();

            //auto connectPort = portToConnectTo(newEvent, parentToCheck)
            ConnectionPort::Ptr connectPort;
            // Get port to connect to (if there is one)
            auto newEvent = event.getEventRelativeTo(parentToCheck);

            //TODO EffectScene and EffectTree parent under one subclass
            ValueTree treeToCheck;
            if (parentToCheck != this)
                treeToCheck = dynamic_cast<Effect *>(parentToCheck)->getTree();
            else
                treeToCheck = tree;
            connectPort = portToConnectTo(newEvent, treeToCheck);

            if (connectPort != nullptr) {
                setHoverComponent(connectPort);
            } else
                setHoverComponent(nullptr);

        }*/
    }
}

void EffectTreeBase::mouseUp(const MouseEvent &event) {
    if (auto l = dynamic_cast<LineComponent *>(event.eventComponent)) {
        if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent)) {
            // Call create on common parent
            auto effect1 = dynamic_cast<EffectTreeBase*>(l->port1->getParentComponent());
            auto effect2 = dynamic_cast<EffectTreeBase*>(port->getParentComponent());

            auto newConnection = ValueTree(CONNECTION_ID);
            newConnection.setProperty(ConnectionLine::IDs::InPort, port, nullptr);
            newConnection.setProperty(ConnectionLine::IDs::OutPort, l->port1, nullptr);
            //auto newConnection = new ConnectionLine(*port, *l->port1);

            if (effect1->getParent() == effect2->getParent()) {
                dynamic_cast<EffectTreeBase*>(effect1->getParent())->getTree().appendChild(newConnection, &undoManager);
            } else if (effect1->getParent() == effect2) {
                dynamic_cast<EffectTreeBase*>(effect2)->getTree().appendChild(newConnection, &undoManager);
            } else if (effect2->getParent() == effect1) {
                dynamic_cast<EffectTreeBase*>(effect1)->getTree().appendChild(newConnection, &undoManager);
            } else {
                std::cout << "what is parent???" << newLine;
                std::cout << "Parent 1: " << l->port1->getParentComponent()->getName() << newLine;
                std::cout << "Parent 2: " << port->getParentComponent()->getName() << newLine;
            }
        }
    }
}

EffectTreeBase::EffectTreeBase(const ValueTree &vt) {
        tree = vt;
        tree.setProperty(IDs::effectTreeBase, this, nullptr);

        setWantsKeyboardFocus(true);

        dragLine.setAlwaysOnTop(true);
        LineComponent::setDragLine(&dragLine);
}

EffectTreeBase::EffectTreeBase(Identifier id) : tree(id) {
    tree.setProperty(IDs::effectTreeBase, this, nullptr);
    setWantsKeyboardFocus(true);

    dragLine.setAlwaysOnTop(true);
    LineComponent::setDragLine(&dragLine);
}

ValueTree EffectTreeBase::storeEffect(ValueTree &tree) {
    ValueTree copy = tree;

    std::cout << "Storing: " << tree.getType().toString() << newLine;

    if (tree.hasType(EFFECT_ID)) {
        // Recurse for children
        copy.removeProperty(Effect::IDs::initialised, nullptr);
        copy.removeProperty(Effect::IDs::connections, nullptr);

        // Set position property
        /*auto pos = Position::fromVar(copy.getProperty(Effect::IDs::pos));
        copy.setProperty("x", pos.x, nullptr);
        copy.setProperty("y", pos.y, nullptr);

        copy.removeProperty(Effect::IDs::pos, nullptr);
*/
        // Store object as ID
        auto ptr = dynamic_cast<Effect*>(tree.getProperty(
                IDs::effectTreeBase).getObject());
        //copy.removeProperty(IDs::effectTreeBase, nullptr);
        copy.setProperty("ID", reinterpret_cast<int64>(ptr), nullptr);
    }

    if (tree.hasType(EFFECTSCENE_ID) || tree.hasType(EFFECT_ID)) {
        copy.removeProperty(IDs::effectTreeBase, nullptr);

        for (int i = 0; i < tree.getNumChildren(); i++) {
            auto child = tree.getChild(i);
            copy.appendChild(child, nullptr);

            if (child.hasType(EFFECT_ID)) {
                storeEffect(child);
            }

            // Register connection
            if (child.hasType(CONNECTION_ID)) {
                // Get data to set
                auto inPortObject = getPropertyFromTree<ConnectionPort>(child, ConnectionLine::IDs::InPort);
                auto outPortObject = getPropertyFromTree<ConnectionPort>(child, ConnectionLine::IDs::OutPort);

                auto effect1 = dynamic_cast<Effect*>(inPortObject->getParentComponent());
                auto effect2 = dynamic_cast<Effect*>(outPortObject->getParentComponent());

                auto inPortID = effect1->getPortID(inPortObject);
                auto outPortID = effect2->getPortID(outPortObject);

                // Set data

                ValueTree connectionToSet(CONNECTION_ID);

                connectionToSet.setProperty("inPortEffect", reinterpret_cast<int64>(effect1), nullptr);
                connectionToSet.setProperty("outPortEffect", reinterpret_cast<int64>(effect2), nullptr);

                connectionToSet.setProperty("inPortID", inPortID, nullptr);
                connectionToSet.setProperty("outPortID", outPortID, nullptr);

                connectionToSet.setProperty("inPortIsInternal",
                        (dynamic_cast<InternalConnectionPort*>(inPortObject) != nullptr), nullptr);
                connectionToSet.setProperty("outPortIsInternal",
                                            (dynamic_cast<InternalConnectionPort*>(outPortObject) != nullptr), nullptr);

                // Set data to ValueTree
                child.copyPropertiesFrom(connectionToSet, nullptr);
            }
        }
    }

    std::cout << "Properties" << newLine;
    for (int i = 0; i < copy.getNumProperties(); i++) {
        std::cout << copy.getPropertyName(i).toString() << newLine;
    }
    std::cout << "Children" << newLine;
    for (int i = 0; i < copy.getNumChildren(); i++) {
        std::cout << copy.getChild(i).getType().toString() << newLine;
    }

    return copy;
}

//todo storageIDs
void EffectTreeBase::loadEffect(ValueTree &parentTree, ValueTree &loadData) {
    ValueTree copy(loadData.getType());

    if (loadData.hasType(EFFECT_ID)) {
        copy.copyPropertiesFrom(loadData, nullptr);
/*
        // Set position property
        int x = loadData.getProperty("x");
        int y = loadData.getProperty("y");
        copy.setProperty(Effect::IDs::pos, Position::toVar(Point<int>(x,y)), nullptr);*/
    }

    // Load child effects

    if (loadData.hasType(EFFECT_ID) || loadData.hasType(EFFECTSCENE_ID)) {
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);
            if (child.hasType(EFFECT_ID)) {
                loadEffect(copy, child);
            }
        }
    }

    // Create effects by adding them to valuetree

    if (loadData.hasType(EFFECT_ID)) {
        parentTree.appendChild(copy, &undoManager);
    } else if (loadData.hasType(EFFECTSCENE_ID)) {
        // Set children of Effectscene, and object property
        auto effectSceneObject = parentTree.getProperty(IDs::effectTreeBase);

        copy.removeAllProperties(nullptr);

        copy.setProperty(IDs::effectTreeBase, effectSceneObject, nullptr);
        parentTree.copyPropertiesAndChildrenFrom(copy, nullptr);
    }



    // After effects created, add connections.

    if (loadData.hasType(EFFECT_ID) || loadData.hasType(EFFECTSCENE_ID)) {
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);
            if (child.hasType(CONNECTION_ID)) {
                // Set data
                ValueTree connectionToSet(CONNECTION_ID);

                int64 inPortEffectID = child.getProperty("inPortEffect");
                int64 outPortEffectID = child.getProperty("outPortEffect");

                auto in = parentTree.getChildWithProperty("ID", inPortEffectID);
                auto out = parentTree.getChildWithProperty("ID", outPortEffectID);

                auto inEffect = getFromTree<Effect>(in);
                auto outEffect = getFromTree<Effect>(out);

                auto inPortIsInternal = in.getProperty("inPortIsInternal");
                auto outPortIsInternal = out.getProperty("outPortIsInternal");

                if (inPortIsInternal || outPortIsInternal) {

                    std::cout << "internal port" << newLine;
                    //todo
                    continue;
                } else {
                    if (inEffect == NULL || outEffect == NULL) {
                        return; //todo return bool?
                    }

                    auto inPort = inEffect->getPortFromID(child.getProperty("inPortID"));
                    auto outPort = outEffect->getPortFromID(child.getProperty("outPortID"));

                    if (inPort == NULL || outPort == NULL) {
                        return;
                    }

                    connectionToSet.setProperty(ConnectionLine::IDs::InPort, inPort, nullptr);
                    connectionToSet.setProperty(ConnectionLine::IDs::OutPort, outPort, nullptr);

                    parentTree.appendChild(connectionToSet, nullptr);
                }
            }
        }
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




Effect::Effect(const ValueTree& vt) : EffectTreeBase(vt) {
    //tree = vt;

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
        node = audioGraph.addNode(move(newProcessor));

        // Create from node:
        setProcessor(node->getProcessor());
    } else {
        // Make edit mode true by default
        setEditMode(true);
    }

    // Set name
    if (isIndividual()) {
        tree.setProperty(IDs::name, processor->getName(), nullptr);
    } else {
        tree.setProperty(IDs::name, "Effect", nullptr);
    }


    int x = tree.getProperty(IDs::x);
    int y = tree.getProperty(IDs::y);
    //Point<int> newPos = Position::fromVar(tree.getProperty(IDs::pos));
    setBounds(x, y, 200,200);

    addAndMakeVisible(resizer);

    // Set tree properties
    tree.addListener(this);

    // Position
    //pos.referTo(tree, IDs::pos, &undoManager);
    setPos(getPosition());

    // Set parent component
    auto parentTree = vt.getParent();
    if (parentTree.isValid()) {
        auto parent = getFromTree<EffectTreeBase>(parentTree);
        parent->addAndMakeVisible(this);
    }

    setupTitle();
    setupMenu();
}

void Effect::setupTitle() {
    Font titleFont(20, Font::FontStyleFlags::bold);
    title.setFont(titleFont);

    title.setEditable(true);
    title.setText(tree.getProperty(IDs::name), dontSendNotification);
    title.setBounds(25, 20, 200, title.getFont().getHeight());

    title.onTextChange = [=]{
        // Name change undoable action
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

    editMenu.addItem("Add Input Port", [=]() {
        addPort(getDefaultBus(), true); resized();
    });
    editMenu.addItem("Add Output Port", [=](){
        addPort(getDefaultBus(), false); resized();
    });
    editMenu.addItem("Toggle Edit Mode", [=]() {
        setEditMode(!editMode);
    });
}

Effect::~Effect()
{
    // Delete processor from graph
    audioGraph.removeNode(node->nodeID);
}

// Processor hasEditor? What to do if processor is a predefined plugin
void Effect::setProcessor(AudioProcessor *processor) {
    setEditMode(false);
    this->processor = processor;

    // Processor settings (how best to do this?)
    node->getProcessor()->setPlayConfigDetails(
            processor->getTotalNumInputChannels(),
            processor->getTotalNumOutputChannels(),
            audioGraph.getSampleRate(),
            audioGraph.getBlockSize());

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

    int numParams = parameters->getParameters(false).size();
    if (numParams > 0) {
        setBounds(getBounds().expanded(40, numParams * 20));
    }

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

        for (auto p : inputPorts) {
            p->setColour(0, Colours::whitesmoke);
        }
        for (auto p : outputPorts) {
            p->setColour(0, Colours::whitesmoke);
        }

        auto size = getBounds().expanded(50);
        setSize(size.getX(), size.getY());
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

        for (auto p : inputPorts) {
            p->setColour(0, Colours::black);
        }
        for (auto p : outputPorts) {
            p->setColour(0, Colours::black);
        }
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

    //todo parent class for dis shit pllssss
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


        auto label = new Label(param->getName(30), param->getName(30));
        label->setFont(Font(15, Font::FontStyleFlags::bold));
        label->setColour(Label::textColourId, Colours::black);

        slider->setTextBoxIsEditable(true);
        slider->setValue(param->getValue(), dontSendNotification);

        auto i = parameters->getParameters(false).indexOf(param);

        slider->setBounds(40 , 40 + (i * 70), 150, 70);
        addAndMakeVisible(slider);

        label->setBounds(slider->getX(), slider->getY() + 5, slider->getWidth(), 20);
        addAndMakeVisible(label);

        slider->hideTextBox(false);
        slider->hideTextBox(true);

        resize(slider->getWidth() + 100, getHeight());
    }
}


void Effect::addPort(AudioProcessor::Bus *bus, bool isInput) {
    auto p = isInput ?
             inputPorts.add(std::make_unique<AudioPort>(isInput)) :
             outputPorts.add(std::make_unique<AudioPort>(isInput));
    p->bus = bus;
    if (editMode){
        p->setColour(p->portColour, Colours::whitesmoke);
    }
    addAndMakeVisible(p);

    resized();

    if (!isIndividual()) {
        addChildComponent(p->internalPort.get());
        Point<int> d;
        d = isInput ? Point<int>(50, 0) : Point<int>(-50, 0);

        p->internalPort->setColour(p->portColour, Colours::whitesmoke);
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
    if (dynamic_cast<Resizer*>(event.originalComponent)) {
        undoManager.beginNewTransaction("resize");
        return;
    }

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
    EffectTreeBase::mouseDown(event);
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
        if (dynamic_cast<Effect*>(event.originalComponent)) {
            // Effect drag
            if (auto newParent = effectToMoveTo(event, tree.getParent())) {
                if (newParent != getFromTree<Effect>(tree.getParent())) {
                    tree.getParent().removeChild(tree, &undoManager);
                    newParent->getTree().appendChild(tree, &undoManager);
                    //hoverOver(newParent);

                    if (newParent != this) {
                        SelectHoverObject::setHoverComponent(newParent);
                    } else {
                        SelectHoverObject::resetHoverObject();
                    }
                }
            }
        } else if (dynamic_cast<ConnectionPort*>(event.originalComponent)) {
            // Line Drag
            EffectTreeBase::mouseDrag(event);
        }
    }
}

void Effect::mouseUp(const MouseEvent &event) {
    setAlwaysOnTop(false);

    // set (undoable) data
    setPos(getBoundsInParent().getPosition());

    if (event.eventComponent == event.originalComponent) {
        if (event.mods.isRightButtonDown() && event.getDistanceFromDragStart() < 10) {
            // open menu
            if (editMode) {
                callMenu(editMenu);
            } else {
                callMenu(menu);
            }
        } else return;
    }

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

        }
        // If component is LineComponent, respond to line drag event
        else if (auto l = dynamic_cast<LineComponent *>(event.eventComponent)) {
            EffectTreeBase::mouseUp(event);
        }
    }

    for (auto i : selected){
        std::cout << i->getName() << newLine;
    }
}

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
            int w = treeWhosePropertyHasChanged.getProperty(IDs::w);
            setSize(w, getHeight());
        } else if (property == IDs::h) {
            int h = treeWhosePropertyHasChanged.getProperty(IDs::h);
            setSize(h, getWidth());
        } else if (property == IDs::name) {
            auto e = getFromTree<Effect>(treeWhosePropertyHasChanged);
            auto newName = treeWhosePropertyHasChanged.getProperty(IDs::name);

            e->setName(newName);
            e->title.setText(newName, sendNotificationAsync);
        }
    }
    Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}

void Effect::valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) {
    if (treeWhoseParentHasChanged == tree) {

    }
}

void Effect::resized() {
    /*if (! undoManager.isPerformingUndoRedo()) {
        int w = tree.getProperty(IDs::w);
        int h = tree.getProperty(IDs::h);
        if (getWidth() != w) {
            tree.setProperty(IDs::w, getWidth(), &undoManager);
        } else if (getHeight() != h) {
            tree.setProperty(IDs::h, getHeight(), &undoManager);
        }
    }*/

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
    tree.setProperty("x", newPos.getX(), &undoManager);
    tree.setProperty("y", newPos.getY(), &undoManager);
}

bool Effect::keyPressed(const KeyPress &key) {
    if (key == KeyPress::deleteKey || key.getKeyCode() == KeyPress::backspaceKey) {
        effectsToDelete.add(this);
        tree.getParent().removeChild(tree, &undoManager);
    }
    return EffectTreeBase::keyPressed(key);
}

void Effect::hoverOver(EffectTreeBase *newParent) {
    if (newParent->getWidth() < getParentWidth() || newParent->getHeight() < getParentHeight()) {
        auto newSize = getBounds().expanded(-20);
        resize(newSize.getX(), newSize.getY());
    } else {
        auto newSize = getBounds().expanded(20);
        resize(newSize.getX(), newSize.getY());
    }
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

AudioPort *Effect::getPortFromID(const int id) {
    if (id < inputPorts.size()) {
        return inputPorts[id];
    } else {
        return outputPorts[id - inputPorts.size()];
    }
}

void Effect::resize(int w, int h) {
    tree.setProperty(IDs::w, w, &undoManager);
    tree.setProperty(IDs::h, h, &undoManager);
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
