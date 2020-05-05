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
//
EffectTreeUpdater EffectTreeBase::updater;

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
ConnectionPort* EffectTreeBase::portToConnectTo(const MouseEvent& event, const ValueTree& effectTree) {

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
    createEffectMenu.addItem("Distortion Effect", std::function<void()>(
            [=]{
                undoManager.beginNewTransaction("Create Distortion Effect");
                newEffect("Distortion Effect", 2);
            }
    ));
    createEffectMenu.addItem("Delay Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Delay Effect");
                newEffect("Delay Effect", 3);
            }
    ));
    createEffectMenu.addItem("Reverb Effect", std::function<void()>(
            [=](){
                undoManager.beginNewTransaction("Create Reverb Effect");
                newEffect("Reverb Effect", 4);
            }
    ));

    return createEffectMenu;
}

ValueTree EffectTreeBase::newEffect(String name, int processorID) {
    ValueTree newEffect(Effect::IDs::EFFECT_ID);

    if (name.isNotEmpty()){
        newEffect.setProperty(Effect::IDs::name, name, nullptr);
    }

    newEffect.setProperty(Effect::IDs::x, menuPos.x, nullptr);
    newEffect.setProperty(Effect::IDs::y, menuPos.y, nullptr);

    if (processorID != -1) {
        newEffect.setProperty(Effect::IDs::processorID, processorID, nullptr);
    }

    this->getTree().appendChild(newEffect, &undoManager);

    auto effect = getFromTree<Effect>(newEffect);
    effect->setEditMode(true);

    return newEffect;
}


void EffectTreeBase::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    // ADD EFFECT
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
            auto e = Effect::createEffect(childWhichHasBeenAdded);
        }
    }
        // ADD CONNECTION
    else if (childWhichHasBeenAdded.hasType(CONNECTION_ID)) {
        ConnectionLine *line;
        // Add connection here
        if (!childWhichHasBeenAdded.hasProperty(ConnectionLine::IDs::ConnectionLineObject)) {
            auto inport = getPropertyFromTree<ConnectionPort>(childWhichHasBeenAdded, ConnectionLine::IDs::InPort);
            auto outport = getPropertyFromTree<ConnectionPort>(childWhichHasBeenAdded, ConnectionLine::IDs::OutPort);


            line = new ConnectionLine(*inport, *outport);
            childWhichHasBeenAdded.setProperty(ConnectionLine::IDs::ConnectionLineObject, line, nullptr);

            auto parent = getFromTree<EffectTreeBase>(parentTree);
            parent->addAndMakeVisible(line);
        } else {
            line = getPropertyFromTree<ConnectionLine>(childWhichHasBeenAdded,
                                                       ConnectionLine::IDs::ConnectionLineObject);
            line->setVisible(true);
        }
        line->toFront(false);
        connectAudio(*line);
    }
}

void EffectTreeBase::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                           int indexFromWhichChildWasRemoved) {

    // EFFECT
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
    }
    // CONNECTION
    else if (childWhichHasBeenRemoved.hasType(CONNECTION_ID)) {
        auto line = getPropertyFromTree<ConnectionLine>(childWhichHasBeenRemoved, ConnectionLine::IDs::ConnectionLineObject);
        line->setVisible(false);

        disconnectAudio(*line);
    }
    // PARAMETER
    else if (childWhichHasBeenRemoved.hasType(PARAMETER_ID)) {
        auto parameter = getFromTree<Parameter>(childWhichHasBeenRemoved);
        parameter->setVisible(false);
    }
}

template<class T>
T *EffectTreeBase::getFromTree(const ValueTree &vt) {
    if (vt.hasProperty(IDs::effectTreeBase)) {
        // Return effect
        return dynamic_cast<T*>(vt.getProperty(IDs::effectTreeBase).getObject());
    } else if (vt.hasProperty(ConnectionLine::IDs::ConnectionLineObject)) {
        // Return connection
        return dynamic_cast<T*>(vt.getProperty(ConnectionLine::IDs::ConnectionLineObject).getObject());
    } else if (vt.hasProperty(Parameter::IDs::parameterObject)) {
        // Return parameter
        return dynamic_cast<T*>(vt.getProperty(Parameter::IDs::parameterObject).getObject());
    }
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
    if (key.getKeyCode() == 'l') {
        std::cout << "Available effects: " << newLine;
        for (auto e : EffectLoader::getEffectsAvailable()) {
            std::cout << e << newLine;
        }
    }
    if (key.getModifiers().isCtrlDown() && key.getKeyCode() == 'p') {
        std::cout << "Clear effects" << newLine;
        for (auto e : EffectLoader::getEffectsAvailable()) {
            EffectLoader::clearEffect(e);
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

    // CTRL
    if (key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown()) {
        if (key.getKeyCode() == 'z') {
            std::cout << "Undo: " << undoManager.getUndoDescription() << newLine;
            undoManager.undo();
        } else if ((key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown())
                   && key.getKeyCode() == 'Z') {
            std::cout << "Redo: " << undoManager.getRedoDescription() << newLine;
            undoManager.redo();
        }

        if (key.getKeyCode() == 's') {
            std::cout << "Save effects" << newLine;
            auto savedState = storeEffect(tree).createXml();
            std::cout << "Save state: " << savedState->toString() << newLine;
            getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, savedState.get());
            getAppProperties().getUserSettings()->saveIfNeeded();
        }
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
    menuPos = getTopLevelComponent()->getMouseXYRelative();
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
        if (auto port = dynamic_cast<ConnectionPort *>(hoverComponent.get()))
        {
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

            // Set edit mode
            copy.setProperty("editMode", effect->isInEditMode(), nullptr);

        } else {
            std::cout << "dat shit is not initialised. do not store" << newLine;
            return ValueTree();
        }
    }

    if (tree.hasType(EFFECTSCENE_ID) || tree.hasType(EFFECT_ID)) {
        copy.removeProperty(IDs::effectTreeBase, nullptr);

        for (int i = 0; i < tree.getNumChildren(); i++) {
            auto child = tree.getChild(i);

            // Register child effect
            if (child.hasType(EFFECT_ID))
            {
                auto childTreeToStore = storeEffect(child);
                if (childTreeToStore.isValid()) {
                    copy.appendChild(childTreeToStore, nullptr);
                }
            }
            // Register parameter
            else if (child.hasType(PARAMETER_ID))
            {
                auto childParamObject = getFromTree<Parameter>(child);
                auto childParam = ValueTree(PARAMETER_ID);

                childParam.setProperty("x", childParamObject->getX(), nullptr);
                childParam.setProperty("y", childParamObject->getY(), nullptr);

                childParam.setProperty("name", childParamObject->getName(), nullptr);
                childParam.setProperty("type", childParamObject->type, nullptr);
                childParam.setProperty("value", childParamObject->getParameter()->getValue(), nullptr);

                if (childParamObject->isConnected()) {
                    childParam.setProperty("connectedParam",
                            childParamObject->getConnectedParameter()->getName(), nullptr);
                }

                copy.appendChild(childParam, nullptr);
            }
            // Register connection
            else if  (child.hasType(CONNECTION_ID))
            {
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

        // Load Parameters
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);
            if (child.hasType(PARAMETER_ID)) {
                ValueTree paramChild(PARAMETER_ID);
                paramChild.copyPropertiesFrom(child, nullptr);

                copy.appendChild(paramChild, nullptr);
            }
        }

        parentTree.appendChild(copy, &undoManager);

        // Load child effects
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);

            if (child.hasType(EFFECT_ID)) {
                loadEffect(copy, child);
            }
        }
    }

    // Add Connections
    if (loadData.hasType(EFFECT_ID) || loadData.hasType(EFFECTSCENE_ID)) {
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);

            // Connect parameters
            if (child.hasType(PARAMETER_ID)) {
                if (child.hasProperty("connectedParam")) {
                    auto connectedParamName = child.getProperty("connectedParam");

                    ValueTree paramToConnect(PARAMETER_ID);
                    for (int i = 0; i < loadData.getNumChildren(); i++) {
                        auto c = loadData.getChild(i).getChildWithProperty("name", connectedParamName);
                        if (c.isValid()) {
                            auto thisParam = getFromTree<Parameter>(child);
                            auto toConnectParam = getFromTree<Parameter>(c);
                            thisParam->connect(toConnectParam);
                        }
                    }
                }
            }

            // Connection
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

Point<int> EffectTreeBase::getMenuPos() const {
    return menuPos;
}

//======================================================================================
// methods to move to manager class

Effect* Effect::createEffect(ValueTree &loadData) {
    auto effect = new Effect();
    loadData.setProperty(EffectTreeBase::IDs::effectTreeBase, effect, nullptr);
    effect->addComponentListener(&updater);

    //this would not be needed
    effect->tree = loadData;

    if (loadData.hasProperty(IDs::processorID)) {
        // Individual Effect
        std::unique_ptr<AudioProcessor> newProcessor;
        int id = loadData.getProperty(IDs::processorID);

        auto currentDevice = deviceManager->getCurrentDeviceTypeObject();

        switch (id) {
            case 0:
                newProcessor = std::make_unique<InputDeviceEffect>(currentDevice->getDeviceNames(true),
                        currentDevice->getIndexOfDevice(deviceManager->getCurrentAudioDevice(),true));
                break;
            case 1:
                newProcessor = std::make_unique<OutputDeviceEffect>(currentDevice->getDeviceNames(false),
                        currentDevice->getIndexOfDevice(deviceManager->getCurrentAudioDevice(),false));
                break;
            case 2:
                newProcessor = std::make_unique<DistortionEffect>();
                break;
            case 3:
                newProcessor = std::make_unique<DelayEffect>();
                break;
            case 4:
                newProcessor = std::make_unique<ReverbEffect>();
                break;
            default:
                std::cout << "ProcessorID not found." << newLine;
        }
        auto node = audioGraph->addNode(move(newProcessor));
        effect->setNode(node);
        // Create from node:
        effect->setProcessor(node->getProcessor());
    }
    else {
        int numInputPorts = loadData.getProperty("numInputPorts", 0);
        int numOutputPorts = loadData.getProperty("numOutputPorts", 0);

        for (int i = 0; i < numInputPorts; i++) {
            effect->addPort(getDefaultBus(), true);
        }
        for (int i = 0; i < numOutputPorts; i++) {
            effect->addPort(getDefaultBus(), false);
        }
    }

    //==============================================================
    // Set edit mode
    if (loadData.hasProperty("editMode")) {
        bool editMode = loadData.getProperty("editMode");
        effect->setEditMode(editMode);
    } else {
        effect->setEditMode(! effect->isIndividual());
    }

    //==============================================================
    // Set name
    if (! loadData.hasProperty(IDs::name)) {
        if (effect->isIndividual()) {
            effect->setName(effect->processor->getName());
        } else {
            effect->setName("Effect");
        }
    }

    //==============================================================
    // Set up parameters

    // Get parameter children already existing to add
    Array<ValueTree> parameterChildren;

    for (int i = 0; i < loadData.getNumChildren(); i++) {
        if (loadData.getChild(i).hasType(PARAMETER_ID)) {
            parameterChildren.add(loadData.getChild(i));
        }
    }

    auto numProcessorParameters = effect->parameters == nullptr
            ? 0 : effect->parameters->getParameters(false).size();
    auto numTreeParameters = parameterChildren.size();

    // If there are processor parameters not registered in tree
    if (numProcessorParameters > numTreeParameters) {
        // Iterate through processor parameters
        for (int i = 0; i < numProcessorParameters; i++) {
            auto processorParam = effect->parameters->getParameters(false)[i];
            // If parameter doesn't already exist in tree
            if (! loadData.getChildWithProperty("name", processorParam->getName(30)).isValid()) {
                auto parameterVT = effect->createParameter(processorParam);

                // Set position
                auto x = (effect->inputPorts.size()) > 0 ? 60 : 30;
                auto y = 30 + i * 50;

                parameterVT.setProperty("x", x, nullptr);
                parameterVT.setProperty("y", y, nullptr);

                // Add parameterVT to tree
                parameterChildren.add(parameterVT);
                effect->tree.appendChild(parameterVT, nullptr);
            }
        }
    }

    // Load parameters from tree
    for (int i = 0; i < loadData.getNumChildren(); i++) {
        if (loadData.getChild(i).hasType(PARAMETER_ID)) {
            auto parameter = effect->loadParameter(effect->tree.getChild(i));
            effect->tree.getChild(i).setProperty(Parameter::IDs::parameterObject, parameter.get(), nullptr);
        }
    }

    //==============================================================
    // Set parent component

    auto parentTree = loadData.getParent();
    if (parentTree.isValid()) {
        auto parent = getFromTree<EffectTreeBase>(parentTree);
        parent->addAndMakeVisible(effect);
    }

    //==============================================================
    // Set up bounds

    int x = loadData.getProperty(IDs::x, 0);
    int y = loadData.getProperty(IDs::y, 0);
    int w = loadData.getProperty(IDs::w, 200);
    int h = loadData.getProperty(IDs::h, 200);

    // Increase to fit ports and parameters

    for (auto p : parameterChildren) {
        auto parameter = getFromTree<Parameter>(p);
        auto parameterBounds = parameter->getBounds();

        w = jmax(parameterBounds.getX() + parameterBounds.getWidth() + 10, w);
        h = jmax(parameterBounds.getY() + parameterBounds.getHeight() + 25, h);
    }

    for (auto p : effect->getPorts()) {
        auto portBounds = p->getBounds();

        w = jmax(portBounds.getX() + portBounds.getWidth(), w);
        h = jmax(portBounds.getY() + portBounds.getHeight(), h);
    }

    // Set new bounds
    effect->setBounds(x, y, w, h);

    //==============================================================
    // Set up other stuff

    effect->setupMenu();
    effect->setupTitle();

    return effect;
}

//======================================================================================

Effect::Effect() : EffectTreeBase(EFFECT_ID) {
    addAndMakeVisible(resizer);
    resizer.setAlwaysOnTop(true);
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

    if (editMode && appState != loading) {
        //title.grabKeyboardFocus();
        title.setWantsKeyboardFocus(true);
        title.showEditor();
    }
}

void Effect::setupMenu() {
    PopupMenu::Item toggleEditMode("Toggle Edit Mode");
    toggleEditMode.setAction([=]() {
        setEditMode(!editMode);
    });

    menu.addItem(toggleEditMode);
    editMenu.addItem(toggleEditMode);

    if (!isIndividual()) {
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
    }


    editMenu.addItem("Change Effect Image..", [=]() {
        FileChooser imgChooser ("Select Effect Image..",
                                File::getSpecialLocation (File::userHomeDirectory),
                                "*.jpg;*.png;*.gif");

        if (imgChooser.browseForFileToOpen())
        {
            image = ImageFileFormat::loadFrom(imgChooser.getResult());
        }
    });

    PopupMenu portSubMenu;
    portSubMenu.addItem("Add Input Port", [=]() {
        addPort(getDefaultBus(), true);
        resized();
    });
    portSubMenu.addItem("Add Output Port", [=](){
        addPort(getDefaultBus(), false);
        resized();
    });
    editMenu.addSubMenu("Add Port..", portSubMenu);

    PopupMenu parameterSubMenu;
    parameterSubMenu.addItem("Add Slider", [=] () {
        auto newParam = createParameter(new MetaParameter("New Parameter"));
        newParam.setProperty("x", getMouseXYRelative().getX(), nullptr);
        newParam.setProperty("y", getMouseXYRelative().getY(), nullptr);
        tree.appendChild(newParam, &undoManager);
        loadParameter(newParam);
    });
    editMenu.addSubMenu("Add Parameter..", parameterSubMenu);

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
        for (int i = 0; i < tree.getNumChildren(); i++) {
            auto c = getFromTree<Component>(tree.getChild(i));
            if (c != nullptr) {
                c->toFront(false);
                c->setInterceptsMouseClicks(true, true);

                if (dynamic_cast<Effect*>(c)) {
                    c->setVisible(true);
                }
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

        bool hideEffects = image.isValid();
        std::cout << "Image valid: " << hideEffects << newLine;

        // Make child effects and connections not editable
        for (int i = 0; i < tree.getNumChildren(); i++) {
            auto c = getFromTree<Component>(tree.getChild(i));
            if (c != nullptr) {
                c->toBack();
                c->setInterceptsMouseClicks(false, false);
                if (dynamic_cast<Effect*>(c)) {
                    c->setVisible(! hideEffects);
                }
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

void Effect::setParameters(const AudioProcessorParameterGroup *group) {
    for (auto param : group->getParameters(false)) {
        addParameterFromProcessorParam(param);
    }
}

Parameter& Effect::addParameterFromProcessorParam(AudioProcessorParameter *param)
{/*
    Parameter* parameterGui;

    auto paramName = param->getName(30).trim();

    ValueTree parameter(PARAMETER_ID);

    parameterGui = new Parameter(param);
    parameter.setProperty(Parameter::IDs::parameterObject, parameterGui, nullptr);

    tree.appendChild(parameter, &undoManager);

    return *parameterGui;*/
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
                menuPos = event.getPosition();
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
    if (dynamic_cast<ParameterPort*>(event.originalComponent)) {
        auto port1 = dynamic_cast<ParameterPort*>(dragLine.getPort1());
        auto port2 = dynamic_cast<ParameterPort*>(hoverComponent.get());

        if (port1 != nullptr && port2 != nullptr) {

            // InPort belongs to Internal parameter, but is child of parent effect.
            auto inPort = port1->isInput ? port2 : port1;
            auto outPort = port1->isInput ? port1 : port2;

            // Connect internal Parameter2 to external Parameter1
            auto e = dynamic_cast<Effect*>(getParentComponent());
            auto inParameter = e->getParameterForPort(inPort);

            auto outParameter = dynamic_cast<Parameter*>(outPort->getParentComponent());

            if (inParameter == nullptr || outParameter == nullptr) {
                std::cout << "Failed to connect parameters!" << newLine;
                dragLine.release(nullptr);
                return;
            }

            // Connect
            outParameter->connect(inParameter);

            auto newConnection = new ConnectionLine(*port1, *port2);
            port1->getDragLineParent()->addAndMakeVisible(newConnection);
        } else {
            dragLine.release(nullptr);
        }
    } else {
        // Call ConnectionPort connect
        EffectTreeBase::mouseUp(event);
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

EffectTreeUpdater::EffectTreeUpdater() {

}

void EffectTreeUpdater::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    if (auto effect = dynamic_cast<Effect*>(&component)) {
        if (wasMoved) {
            std::cout << "Effect move update" << newLine;
            effect->getTree().setProperty(Effect::IDs::x, effect->getX(), undoManager);
            effect->getTree().setProperty(Effect::IDs::y, effect->getY(), undoManager);
        }
        else if (wasResized) {
            std::cout << "Effect size update" << newLine;
            effect->getTree().setProperty(Effect::IDs::w, effect->getWidth(), undoManager);
            effect->getTree().setProperty(Effect::IDs::h, effect->getHeight(), undoManager);
        }
    } else if (auto parameter = dynamic_cast<Parameter*>(&component)) {
        auto effect = dynamic_cast<Effect*>(parameter->getParentComponent());
        if (wasMoved) {
            std::cout << "Parameter move update" << newLine;
            auto parameterTree = effect->getTree().getChildWithProperty(Parameter::IDs::parameterObject, parameter);
            parameterTree.setProperty("x", component.getX(), nullptr);
            parameterTree.setProperty("y", component.getY(), nullptr);
        }
    }



    ComponentListener::componentMovedOrResized(component, wasMoved, wasResized);
}

void EffectTreeUpdater::componentNameChanged(Component &component) {

    ComponentListener::componentNameChanged(component);
}

void EffectTreeUpdater::setUndoManager(UndoManager &um) {
    undoManager = &um;
}

void EffectTreeUpdater::componentChildrenChanged(Component &component) {
    for (auto c : component.getChildren()) {
        c->addComponentListener(this);
    }
    ComponentListener::componentChildrenChanged(component);
}
