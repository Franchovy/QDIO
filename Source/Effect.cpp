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

// Static members
EffectTreeBase::AppState EffectTreeBase::appState = neutral;
AudioProcessorGraph* EffectTreeBase::audioGraph = nullptr;
AudioProcessorPlayer* EffectTreeBase::processorPlayer = nullptr;
AudioDeviceManager* EffectTreeBase::deviceManager = nullptr;
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

    ValueTree effectTreeToCheck;
    if (effectTree.getParent().hasType(EFFECTSCENE_ID)
            && dynamic_cast<AudioPort*>(event.originalComponent))
    {
        effectTreeToCheck = effectTree.getParent();
    } else {
        effectTreeToCheck = effectTree;
    }

    // Check children for a match
    for (int i = 0; i < effectTreeToCheck.getNumChildren(); i++) {
        auto e = getFromTree<Effect>(effectTreeToCheck.getChild(i));

        if (e == nullptr)
            continue;

        // Filter self effect if AudioPort
        if (auto p = dynamic_cast<AudioPort*>(event.originalComponent))
            if (p->getParentComponent() == e)
                continue;

        auto localPos = e->getLocalPoint(event.originalComponent, event.getPosition());

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
    for (auto e : effectsToDelete) {
        e->getTree().removeAllProperties(nullptr);
    }

    effectsToDelete.clear();
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
                undoManager.beginNewTransaction("Create Effect");
                newEffect("Effect", -1);
            }));
    createEffectMenu.addItem("Input Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Input Effect");
                newEffect("Input Device", 0);
            }));
    createEffectMenu.addItem("Output Device", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Output Effect");
                newEffect("Output Device", 1);
            }));
    createEffectMenu.addItem("Delay Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Delay Effect");
                newEffect("Delay Effect", 3);
            }
    ));
    createEffectMenu.addItem("Distortion Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Distortion Effect");
                newEffect("Distortion Effect", 2);
            }
    ));

    return createEffectMenu;
}

ValueTree EffectTreeBase::newEffect(String name, int processorID) {
    ValueTree newEffect(Effect::IDs::EFFECT_ID);

    if (name.isNotEmpty()){
        newEffect.setProperty(Effect::IDs::name, name, nullptr);
    }

    newEffect.setProperty(Effect::IDs::x, getMouseXYRelative().x, nullptr);
    newEffect.setProperty(Effect::IDs::y, getMouseXYRelative().y, nullptr);

    if (processorID != -1) {
        newEffect.setProperty(Effect::IDs::processorID, processorID, nullptr);
    }

    this->getTree().appendChild(newEffect, &undoManager);
    return newEffect;
}
/*
void EffectTreeBase::newEffect(ValueTree& tree) {
    int processorID;
    if (tree.hasProperty(Effect::IDs::processorID)) {
        processorID = tree.getProperty(Effect::IDs::processorID);
    } else {
        processorID = -1;
    }
    String name;
    if (tree.hasProperty(Effect::IDs::name)) {
        name = tree.getProperty(Effect::IDs::name);
    } else {
        name = "Effect";
    }
    newEffect(name, processorID);

}*/


void EffectTreeBase::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    if (getFromTree<Effect>(childWhichHasBeenAdded) != nullptr) {
        std::cout << "Add " << getFromTree<Effect>(childWhichHasBeenAdded)->getName() <<  " to " << parentTree.getType().toString() << newLine;
    }

    // if effect has been created already
    if (childWhichHasBeenAdded.hasType(EFFECT_ID)) {
        if (childWhichHasBeenAdded.hasProperty(Effect::IDs::initialised)) {
            if (auto e = getFromTree<Effect>(childWhichHasBeenAdded)) {
                e->setVisible(true);
                // Adjust pos
                if (auto parent = getFromTree<EffectTreeBase>(parentTree)) {
                    if (!parent->getChildren().contains(e)) {
                        parent->addAndMakeVisible(e);

                        e->setTopLeftPosition(parent->getLocalPoint(e, e->getPosition()));
                    }
                }
            }
        } else {
            // Initialise VT
            childWhichHasBeenAdded.setProperty(Effect::IDs::initialised, true, &undoManager);
            // Create new effect
            auto e = new Effect(childWhichHasBeenAdded);
            childWhichHasBeenAdded.setProperty(IDs::effectTreeBase, e, nullptr);

            if (auto parent = getFromTree<EffectTreeBase>(parentTree)) {
                parent->addAndMakeVisible(e);
            }
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
        line->toFront(false);
        connectAudio(*line);
    }
}

void EffectTreeBase::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                           int indexFromWhichChildWasRemoved) {
    std::cout << "VT child removed" << newLine;
    if (childWhichHasBeenRemoved.hasType(EFFECT_ID)) {
        if (auto e = getFromTree<Effect>(childWhichHasBeenRemoved)) {
            if (auto parent = getFromTree<EffectTreeBase>(parentTree)) {

                // Remove connections
                //todo use connections refarray as Effect member for fast access
                int childrenRemoved = 0; // Used for index-correcting
                for (int i = 0; (i - childrenRemoved) < parentTree.getNumChildren(); i ++) {
                    auto child = parentTree.getChild(i - childrenRemoved);
                    if (child.hasType(CONNECTION_ID)) {
                        auto connection = getPropertyFromTree<ConnectionLine>(child, ConnectionLine::IDs::ConnectionLineObject);

                        if (e->hasConnection(connection)) {
                            parentTree.removeChild(child, &undoManager);
                            childrenRemoved++;
                        }
                    }
                }

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

#ifdef DEBUG_UTILITIES
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
    if (key.getKeyCode() == 'p') {
        std::cout << "Main Position: " << getMouseXYRelative().toString() << newLine;
        std::cout << "Relative Position: " << getComponentAt(getMouseXYRelative())->getLocalPoint(this, getMouseXYRelative()).toString() << newLine;
    }
    if (key.getKeyCode() == 'e') {
        if (auto e = dynamic_cast<EffectTreeBase*>(getComponentAt(getMouseXYRelative()))) {
            auto tree = e->getTree();
            std::cout << "Properties: " << newLine;
            for (int i = 0; i < tree.getNumProperties(); i++) {
                auto property = tree.getPropertyName(i);
                std::cout << property.toString() << ": " << tree.getProperty(property).toString() << newLine;
            }
            if (tree.getNumChildren() > 0) {
                std::cout << "Children: " << newLine;
                for (int i = 0; i < tree.getNumChildren(); i++) {
                    auto child = tree.getChild(i);

                    std::cout << child.getType().toString() << newLine;
                }
            }
        }
    }
#endif

    if (key.getKeyCode() == KeyPress::spaceKey) {
        if (deviceManager->getCurrentAudioDevice()->isPlaying()) {
            deviceManager->getCurrentAudioDevice()->stop();
        } else {
            deviceManager->getCurrentAudioDevice()->start(processorPlayer);
        }
    }

    if (key.getKeyCode() == KeyPress::deleteKey || key.getKeyCode() == KeyPress::backspaceKey) {
        for (const auto& selectedItem : selected.getItemArray()) {
            if (auto l = dynamic_cast<ConnectionLine*>(selectedItem.get())) {
                auto lineTree = tree.getChildWithProperty(ConnectionLine::IDs::ConnectionLineObject, l);
                tree.removeChild(lineTree, &undoManager);
            } else if (auto e = dynamic_cast<Effect*>(selectedItem.get())) {
                effectsToDelete.add(e);
                e->getTree().getParent().removeChild(e->getTree(), &undoManager);
            }
        }
    }

    if ((key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown())
            && key.getKeyCode() == 'z') {
        std::cout << "Undo: " << undoManager.getUndoDescription() << newLine;
        undoManager.undo();
    } else if ((key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown())
            && key.getKeyCode() == 'Z') {
        std::cout << "Redo: " << undoManager.getRedoDescription() << newLine;
        undoManager.redo();
    }

    if ((key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown())
            && key.getKeyCode() == 's') {
        std::cout << "Save effects" << newLine;

        /*auto savedState = storeEffect(tree).createXml();
        std::cout << "Save state: " << savedState->toString() << newLine;
        getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, savedState.get());
        getAppProperties().getUserSettings()->saveIfNeeded();*/
    }
}

EffectTreeBase::~EffectTreeBase() {
    for (int i = 0; i < tree.getNumChildren(); i++) {

        tree.getChild(i).removeProperty(IDs::effectTreeBase, nullptr);
        tree.removeChild(i, nullptr);
    }
    if (getReferenceCount() > 0) {
        incReferenceCount();
        tree.removeAllProperties(nullptr);
        decReferenceCountWithoutDeleting();
    }
}


void EffectTreeBase::callMenu(PopupMenu& m) {
    // Execute result
    int result = m.show();
}

void EffectTreeBase::mouseDown(const MouseEvent &event) {
    std::cout << "Begin new transaction" << newLine;
    undoManager.beginNewTransaction(getName());

    if (auto p = dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        dragLine.startDrag(p, event);
    }

}

void EffectTreeBase::mouseDrag(const MouseEvent &event) {
    if (lasso.isVisible())
        lasso.dragLasso(event);

    if (event.mods.isLeftButtonDown()) {
        // Line drag
        if (dynamic_cast<ConnectionPort*>(event.originalComponent)) {
            //TODO efficiency
            auto port = portToConnectTo(event, tree);

            if (port != nullptr) {
                SelectHoverObject::setHoverComponent(port);
            } else {
                SelectHoverObject::resetHoverObject();
            }
            dragLine.drag(event);
        }
    }
}

void EffectTreeBase::mouseUp(const MouseEvent &event) {
    if (dynamic_cast<ConnectionPort*>(event.originalComponent)) {
        if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent.get())) {
            // Call create on common parent
            newConnection(port, dragLine.getPort1());
        }

        dragLine.release(nullptr);
    }
}

EffectTreeBase::EffectTreeBase(const ValueTree &vt) {
        tree = vt;
        tree.setProperty(IDs::effectTreeBase, this, nullptr);

        setWantsKeyboardFocus(true);

        dragLine.setAlwaysOnTop(true);
}

EffectTreeBase::EffectTreeBase(Identifier id) : tree(id) {
    tree.setProperty(IDs::effectTreeBase, this, nullptr);
    setWantsKeyboardFocus(true);

    dragLine.setAlwaysOnTop(true);
}

ValueTree EffectTreeBase::storeEffect(const ValueTree &tree) {
    ValueTree copy(tree.getType());
    copy.copyPropertiesFrom(tree, nullptr);

    std::cout << "Storing: " << tree.getType().toString() << newLine;

    if (tree.hasType(EFFECT_ID)) {
        // Remove unused properties
        copy.removeProperty(Effect::IDs::initialised, nullptr);
        copy.removeProperty(Effect::IDs::connections, nullptr);

        // Set properties based on object
        if (auto effect = dynamic_cast<Effect*>(tree.getProperty(
                IDs::effectTreeBase).getObject())) {

            // Set size property
            copy.setProperty(Effect::IDs::w, effect->getWidth(), nullptr);
            std::cout << effect->getWidth() << " " << effect->getHeight() << newLine;
            copy.setProperty(Effect::IDs::h, effect->getHeight(), nullptr);

            // Set ID property (for port connections)
            copy.setProperty("ID", reinterpret_cast<int64>(effect), nullptr);

            // Set num ports
            copy.setProperty("numInputPorts", effect->getNumInputs(), nullptr);
            copy.setProperty("numOutputPorts", effect->getNumOutputs(), nullptr);

            // Save parameter info
            copy.appendChild(effect->storeParameters(), nullptr);

        } else {
            std::cout << "dat shit is not initialised. do not store" << newLine;
            return ValueTree();
        }
    }

    if (tree.hasType(EFFECTSCENE_ID) || tree.hasType(EFFECT_ID)) {
        copy.removeProperty(IDs::effectTreeBase, nullptr);

        for (int i = 0; i < tree.getNumChildren(); i++) {
            auto child = tree.getChild(i);

            if (child.hasType(EFFECT_ID)) {
                auto childTreeToStore = storeEffect(child);
                if (childTreeToStore.isValid()) {
                    copy.appendChild(childTreeToStore, nullptr);
                }
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
                copy.appendChild(connectionToSet, nullptr);
            }
        }
    }

    return copy;
}

//todo storageIDs
void EffectTreeBase::loadEffect(ValueTree &parentTree, const ValueTree &loadData) {
    ValueTree copy(loadData.getType());

    // Effectscene added first
    if (loadData.hasType(EFFECTSCENE_ID)) {
        // Set children of Effectscene, and object property
        auto effectSceneObject = parentTree.getProperty(IDs::effectTreeBase);

        copy.setProperty(IDs::effectTreeBase, effectSceneObject, nullptr);
        parentTree.copyPropertiesAndChildrenFrom(copy, nullptr);

        // Load child effects
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);
            if (child.hasType(EFFECT_ID)) {
                loadEffect(parentTree, child);
            }
        }
    }
    // Create effects by adding them to valuetree
    else if (loadData.hasType(EFFECT_ID)) {
        copy.copyPropertiesFrom(loadData, nullptr);
        parentTree.appendChild(copy, &undoManager);

        // Load child effects
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);
            if (child.hasType(EFFECT_ID)) {
                loadEffect(copy, child);
            }
        }

        // Load parameters
        auto effect = getFromTree<Effect>(copy);
        if (effect != nullptr && loadData.getChildWithName("parameters").isValid()) {
            effect->loadParameters(loadData.getChildWithName("parameters"));
        }

        //loadData.getChild("parameters")
    }

    // Add Connections
    if (loadData.hasType(EFFECT_ID) || loadData.hasType(EFFECTSCENE_ID)) {
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);
            if (child.hasType(CONNECTION_ID)) {
                // Set data
                ValueTree connectionToSet(CONNECTION_ID);

                int64 inPortEffectID = child.getProperty("inPortEffect");
                int64 outPortEffectID = child.getProperty("outPortEffect");

                bool inPortIsInternal = child.getProperty("inPortIsInternal");
                bool outPortIsInternal = child.getProperty("outPortIsInternal");

                ValueTree in;
                ValueTree out;
                if (parentTree.getChildWithProperty("ID", inPortEffectID).isValid()) {
                    in = parentTree.getChildWithProperty("ID", inPortEffectID);
                } else {
                    in = copy.getChildWithProperty("ID", inPortEffectID);
                }
                if (parentTree.getChildWithProperty("ID", outPortEffectID).isValid()) {
                    out = parentTree.getChildWithProperty("ID", outPortEffectID);
                } else {
                    out = copy.getChildWithProperty("ID", outPortEffectID);
                }

                auto inEffect = getFromTree<Effect>(in);
                auto outEffect = getFromTree<Effect>(out);

                if (inEffect == NULL || outEffect == NULL) {
                    return; //todo return bool?
                }

                auto inPort = inEffect->getPortFromID(child.getProperty("inPortID"), inPortIsInternal);
                auto outPort = outEffect->getPortFromID(child.getProperty("outPortID"), outPortIsInternal);

                if (inPort == NULL || outPort == NULL) {
                    return;
                }

                connectionToSet.setProperty(ConnectionLine::IDs::InPort, inPort, nullptr);
                connectionToSet.setProperty(ConnectionLine::IDs::OutPort, outPort, nullptr);

                if ((inPortIsInternal || outPortIsInternal)
                        || (in.getParent() == copy && out.getParent() == copy)) {
                    copy.appendChild(connectionToSet, nullptr);
                } else {
                    parentTree.appendChild(connectionToSet, nullptr);
                }
            }
        }
    }
}

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

ValueTree EffectTreeBase::newConnection(ConnectionPort::Ptr port1, ConnectionPort::Ptr port2) {
    ConnectionPort::Ptr inPort;
    ConnectionPort::Ptr outPort;
    if (port1->isInput) {
        inPort = port1;
        outPort = port2;
    } else {
        inPort = port2;
        outPort = port1;
    }

    // Get port parents - Remember that input port is for output effect and vice versa.
    auto effect1 = dynamic_cast<Effect*>(outPort->getParentComponent());
    auto effect2 = dynamic_cast<Effect*>(inPort->getParentComponent());

    auto newConnection = ValueTree(CONNECTION_ID);
    newConnection.setProperty(ConnectionLine::IDs::InPort, inPort.get(), nullptr);
    newConnection.setProperty(ConnectionLine::IDs::OutPort, outPort.get(), nullptr);
    //auto newConnection = new ConnectionLine(*port, *l->port1);
    std::cout << "Effect1: " << effect1 << newLine;

    if (effect1->getParent() == effect2->getParent()) {
        if (effect1 == effect2) {
            //todo fuckin motherfuckin dumbass exception case
            std::cout << "oh, fuck you.." << newLine;
        }

        dynamic_cast<EffectTreeBase*>(effect1->getParent())->getTree().appendChild(newConnection, &undoManager);
    } else if (effect1->getParent() == effect2) {
        dynamic_cast<EffectTreeBase*>(effect2)->getTree().appendChild(newConnection, &undoManager);
    } else if (effect2->getParent() == effect1) {
        dynamic_cast<EffectTreeBase*>(effect1)->getTree().appendChild(newConnection, &undoManager);
    }

    return newConnection;
}

Effect::Effect(const ValueTree& vt) : EffectTreeBase(vt) {
    if (vt.hasProperty(IDs::processorID)) {

        // Individual Effect
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

        node->incReferenceCount();

        // Create from node:
        setProcessor(node->getProcessor());
    }
    // Custom Effect
    else {
        int numInputPorts = tree.getProperty("numInputPorts", 0);
        int numOutputPorts = tree.getProperty("numOutputPorts", 0);

        for (int i = 0; i < numInputPorts; i++) {
            addPort(getDefaultBus(), true);
        }
        for (int i = 0; i < numOutputPorts; i++) {
            addPort(getDefaultBus(), false);
        }

        // Make edit mode true by default
        setEditMode(true);

        parameters = new AudioProcessorParameterGroup();
    }

    // Set name
    if (! tree.hasProperty(IDs::name)) {
        if (isIndividual()) {
            tree.setProperty(IDs::name, processor->getName(), nullptr);
        } else {
            tree.setProperty(IDs::name, "Effect", nullptr);
        }
    }

    int x = tree.getProperty(IDs::x, 0);
    int y = tree.getProperty(IDs::y, 0);
    int w = tree.getProperty(IDs::w, 200);
    int h = tree.getProperty(IDs::h, 200);

    //Point<int> newPos = Position::fromVar(tree.getProperty(IDs::pos));
    setBounds(x, y, w, h);

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

    if (! tree.hasProperty(IDs::processorID) && appState != loading) {
        //title.grabKeyboardFocus();
        title.setWantsKeyboardFocus(true);
        title.showEditor();
    }
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
/*

    PopupMenu parameterSubMenu;
    parameterSubMenu.addItem("Add Slider", [=] () {
        addParameter("Slider");
    });
    parameterSubMenu.add


    editMenu.addItem("Add Parameter")
    */
    editMenu.addItem("Toggle Edit Mode", [=]() {
        setEditMode(!editMode);
    });
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

    auto parameterGui = new Parameter(param);
    addAndMakeVisible(parameterGui);

    auto i = parameters->getParameters(false).indexOf(param);
    parameterGui->setTopLeftPosition(60, 70 + i * 50);

    setSize(parameterGui->getWidth() + 50, 50 + parameters->getParameters(false).size() * 50);
/*
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
        slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

        auto label = new Label(param->getName(30), param->getName(30));
        label->setFont(Font(15, Font::FontStyleFlags::bold));
        label->setColour(Label::textColourId, Colours::black);

        slider->setTextBoxIsEditable(true);
        slider->setValue(param->getValue(), dontSendNotification);

        auto i = parameters->getParameters(false).indexOf(param);

        slider->setBounds(40 , 40 + (i * 70), 100, 70);
        addAndMakeVisible(slider);

        label->setBounds(slider->getX(), slider->getY() + 5, slider->getWidth(), 20);
        addAndMakeVisible(label);

        slider->hideTextBox(false);
        slider->hideTextBox(true);

        resize(slider->getWidth() + 100, getHeight());
    }*/
}


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

void Effect::mouseDown(const MouseEvent &event) {
    if (dynamic_cast<Resizer*>(event.originalComponent)) {
        undoManager.beginNewTransaction("resize");
        return;
    }

    if (event.mods.isLeftButtonDown()) {
        //todo static lasso functionality?

        // Drag
        setAlwaysOnTop(true);
        if (event.mods.isLeftButtonDown()) {
            dragger.startDraggingComponent(this, event);
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
                std::cout << "new parent: " << newParent->getName() << newLine;
                if (newParent != getFromTree<Effect>(tree.getParent())) {
                    reassignNewParent(newParent);

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
        if (event.getDistanceFromDragStart() < 10) {
            if (event.mods.isLeftButtonDown() && ! event.mods.isCtrlDown()) {
                addSelectObject(this);
            }
            if (event.mods.isRightButtonDown() ||
                       event.mods.isCtrlDown()) {
                // open menu
                if (editMode) {
                    callMenu(editMenu);
                } else {
                    callMenu(menu);
                }
            }
        }
    }

    /*if (lasso.isVisible())
        lasso.endLasso();*/

    EffectTreeBase::mouseUp(event);
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

    if (! undoManager.isPerformingUndoRedo()) {
        tree.setProperty(IDs::w, getWidth(), &undoManager);
        tree.setProperty(IDs::h, getHeight(), &undoManager);
    }

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
    std::cout << "Effect paint" << newLine;

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

void Effect::reassignNewParent(EffectTreeBase* newParent) {
    auto parent = tree.getParent();



    parent.removeChild(tree, &undoManager);
    newParent->getTree().appendChild(tree, &undoManager);

}

void Effect::setParent(EffectTreeBase &parent) {
    parent.getTree().appendChild(tree, &undoManager);
}

void Effect::setPos(Point<int> newPos) {
    tree.setProperty("x", newPos.getX(), &undoManager);
    tree.setProperty("y", newPos.getY(), &undoManager);
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

void Effect::resize(int w, int h) {
    tree.setProperty(IDs::w, w, &undoManager);
    tree.setProperty(IDs::h, h, &undoManager);
}

bool Effect::hasProcessor(AudioProcessor *processor) {
    return processor == this->processor;
}

void Effect::updateEffectProcessor(AudioProcessor *processorToUpdate, ValueTree treeToSearch) {
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
}

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

ValueTree Effect::storeParameters() {
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
