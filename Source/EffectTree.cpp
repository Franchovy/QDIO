/*
  ==============================================================================

    EffectTree.cpp
    Created: 8 May 2020 9:42:07am
    Author:  maxime

  ==============================================================================
*/

#include "EffectTree.h"

Identifier EffectTree::IDs::component = "component";

EffectTree::EffectTree(EffectTreeBase* effectScene)
    : effectTree(EFFECTSCENE_ID)
    , undoManager(&effectScene->getUndoManager())
{
    effectTree.setProperty(IDs::component, effectScene, nullptr);
    effectTree.addListener(this);
    effectScene->addComponentListener(this);
}


Effect* EffectTree::loadEffect(ValueTree tree) {
    effectTree.appendChild(tree, undoManager);
    auto effect = new Effect();
    tree.setProperty(IDs::component, effect, nullptr);
    effect->addComponentListener(this);

    if (tree.hasProperty(Effect::IDs::processorID)) {
        // Individual Effect
        std::unique_ptr<AudioProcessor> newProcessor;
        int id = tree.getProperty(Effect::IDs::processorID);

        auto currentDeviceType = EffectTreeBase::getDeviceManager()->getCurrentDeviceTypeObject();
        auto currentAudioDevice = EffectTreeBase::getDeviceManager()->getCurrentAudioDevice();

        switch (id) {
            case 0:
                newProcessor = std::make_unique<InputDeviceEffect>(currentDeviceType->getDeviceNames(true),
                                                                   currentDeviceType->getIndexOfDevice(currentAudioDevice, true));
                break;
            case 1:
                newProcessor = std::make_unique<OutputDeviceEffect>(currentDeviceType->getDeviceNames(false),
                                                                    currentDeviceType->getIndexOfDevice(currentAudioDevice, false));
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
        auto node = EffectTreeBase::getAudioGraph()->addNode(move(newProcessor));
        effect->setNode(node);
        // Create from node:
        effect->setProcessor(node->getProcessor());
    }
    else {
        int numInputPorts = tree.getProperty("numInputPorts", 0);
        int numOutputPorts = tree.getProperty("numOutputPorts", 0);

        for (int i = 0; i < numInputPorts; i++) {
            effect->addPort(Effect::getDefaultBus(), true);
        }
        for (int i = 0; i < numOutputPorts; i++) {
            effect->addPort(Effect::getDefaultBus(), false);
        }
    }

    //==============================================================
    // Set edit mode
    if (tree.hasProperty("editMode")) {
        bool editMode = tree.getProperty("editMode");
        effect->setEditMode(editMode);
    } else {
        effect->setEditMode(! effect->isIndividual());
    }

    //==============================================================
    // Set name
    if (! tree.hasProperty(Effect::IDs::name)) {
        if (effect->isIndividual()) {
            effect->setName(effect->getProcessor()->getName());
        } else {
            effect->setName("Effect");
        }
    } else {
        effect->setName(tree.getProperty(Effect::IDs::name).toString());
    }

    //==============================================================
    // Set up parameters

    // Get parameter children already existing to add
    Array<ValueTree> parameterChildren;

    for (int i = 0; i < tree.getNumChildren(); i++) {
        if (tree.getChild(i).hasType(PARAMETER_ID)) {
            parameterChildren.add(tree.getChild(i));
        }
    }

    auto numProcessorParameters = effect->getParameters(false).size();
    auto numTreeParameters = parameterChildren.size();

    // If there are processor parameters not registered in tree
    if (numProcessorParameters > numTreeParameters) {
        // Iterate through processor parameters
        for (int i = 0; i < numProcessorParameters; i++) {
            auto processorParam = effect->getParameters(false)[i];
            // If parameter doesn't already exist in tree
            if (! tree.getChildWithProperty("name", processorParam->getName(30)).isValid()) {
                auto parameterVT = effect->createParameter(processorParam);

                // Set position
                auto x = (effect->getNumInputs()) > 0 ? 60 : 30;
                auto y = 30 + i * 50;

                parameterVT.setProperty("x", x, nullptr);
                parameterVT.setProperty("y", y, nullptr);

                // Add parameterVT to tree
                parameterChildren.add(parameterVT);
                tree.appendChild(parameterVT, nullptr);
            }
        }
    }

    // Load parameters from tree
    for (int i = 0; i < tree.getNumChildren(); i++) {
        if (tree.getChild(i).hasType(PARAMETER_ID)) {
            loadParameter(effect, tree.getChild(i));
        }
    }

    //==============================================================
    // Set parent component

    auto parentTree = tree.getParent();
    if (parentTree.isValid()) {
        auto parent = getFromTree<EffectTreeBase>(parentTree);
        parent->addAndMakeVisible(effect);
    }

    //==============================================================
    // Set up bounds

    int x = tree.getProperty(Effect::IDs::x, 0);
    int y = tree.getProperty(Effect::IDs::y, 0);
    int w = tree.getProperty(Effect::IDs::w, 200);
    int h = tree.getProperty(Effect::IDs::h, 200);

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

ValueTree EffectTree::newEffect(String name, Point<int> pos, int processorID) {
    ValueTree newEffect(Effect::IDs::EFFECT_ID);

    if (name.isNotEmpty()){
        newEffect.setProperty(Effect::IDs::name, name, nullptr);
    }

    newEffect.setProperty(Effect::IDs::x, pos.x, nullptr);
    newEffect.setProperty(Effect::IDs::y, pos.y, nullptr);

    if (processorID != -1) {
        newEffect.setProperty(Effect::IDs::processorID, processorID, nullptr);
    }

    //this->getTree().appendChild(newEffect, &undoManager);

    //todo set edit mode
    /*auto effect = getFromTree<Effect>(newEffect);
    effect->setEditMode(true);*/

    return newEffect;
}

/*ValueTree EffectTree::newConnection(ConnectionPort::Ptr port1, ConnectionPort::Ptr port2) {
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
        }

        getTree(dynamic_cast<EffectTreeBase*>(effect1->getParent())).appendChild(newConnection, undoManager);
    } else if (effect1->getParent() == effect2) {
        getTree(dynamic_cast<EffectTreeBase*>(effect2)).appendChild(newConnection, undoManager);
    } else if (effect2->getParent() == effect1) {
        getTree(dynamic_cast<EffectTreeBase*>(effect1)).appendChild(newConnection, undoManager);
    }

    return newConnection;
}*/

/*
ValueTree EffectTree::newParameter() {
    return ValueTree();
}*/


void EffectTree::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    if (! undoManager->isPerformingUndoRedo()) {
        if (auto effect = dynamic_cast<Effect*>(&component)) {
            if (! getTree(effect).isValid()) {
                return;
            }

            if (wasMoved) {
                getTree(effect).setProperty(Effect::IDs::x, effect->getX(), undoManager);
                getTree(effect).setProperty(Effect::IDs::y, effect->getY(), undoManager);
            }
            else if (wasResized) {
                getTree(effect).setProperty(Effect::IDs::w, effect->getWidth(), undoManager);
                getTree(effect).setProperty(Effect::IDs::h, effect->getHeight(), undoManager);
            }
        } else if (auto parameter = dynamic_cast<Parameter*>(&component)) {
            auto effect = dynamic_cast<Effect*>(parameter->getParentComponent());
            jassert(effect != nullptr);

            auto parameterTree = getTree(effect).getChildWithProperty(IDs::component, parameter);

            if (wasMoved) {
                parameterTree.setProperty("x", component.getX(), nullptr);
                parameterTree.setProperty("y", component.getY(), nullptr);
            }
        }
    } else {
        std::cout << "Performing undo/redo, not updating valueTree!" << newLine;
    }
    ComponentListener::componentMovedOrResized(component, wasMoved, wasResized);
}

void EffectTree::componentNameChanged(Component &component) {
    if (auto e = dynamic_cast<Effect*>(&component)) {
        getTree(e).setProperty(Effect::IDs::name, component.getName(), undoManager);
    }

    ComponentListener::componentNameChanged(component);
}

void EffectTree::componentChildrenChanged(Component &component) {
    // Effect or EffectScene
    if (auto effectTreeBase = dynamic_cast<EffectTreeBase*>(&component)) {
        std::cout << "componentChildrenChanged effect" << newLine;
        auto effectTree = getTree(effectTreeBase);
        jassert(effectTree.isValid());

        for (auto c : component.getChildren()) {

            //todo move
            c->addComponentListener(this);

            auto child = dynamic_cast<SelectHoverObject*>(c);

            if (child != nullptr) {
                auto childTree = effectTree.getChildWithProperty(IDs::component, child);

                if (childTree.isValid()) {

                } else {
                    // Create ConnectionLine tree
                    if (auto line = dynamic_cast<ConnectionLine *>(c)) {
                        ValueTree newLineTree(CONNECTION_ID);
                        newLineTree.setProperty(IDs::component, line, nullptr);
                        effectTree.appendChild(newLineTree, undoManager);
                        //c->addComponentListener(this);
                    }
                    // Create Parameter Tree
                    else if (auto parameter = dynamic_cast<Parameter *>(c)) {
                        ValueTree newParameterTree(PARAMETER_ID);
                        newParameterTree.setProperty(IDs::component, parameter, nullptr);
                        effectTree.appendChild(newParameterTree, undoManager);
                        //c->addComponentListener(this);
                    }
                }
            }
        }
    }
    // ConnectionLine
    else if (auto line = dynamic_cast<ConnectionLine*>(&component)) {
        std::cout << "componentChildrenChanged line" << newLine;
    }
    // Parameter
    else if (auto parameter = dynamic_cast<Parameter*>(&component)) {
        std::cout << "componentChildrenChanged parameter" << newLine;
    } else {
        std::cout << "componentChildrenChanged something else" << newLine;
    }

    ComponentListener::componentChildrenChanged(component);
}


void EffectTree::componentEnablementChanged(Component &component) {
    if (auto line = dynamic_cast<ConnectionLine*>(&component)) {
        if (line->isEnabled()) {
            // Line is connected
            auto lineTree = getTree(line);
            jassert(lineTree.isValid());
        } else {
            // Line is disconnected

        }

    }

    ComponentListener::componentEnablementChanged(component);
}


ValueTree Effect::createParameter(AudioProcessorParameter *param) {
    ValueTree parameterTree(PARAMETER_ID);
    parameterTree.setProperty("name", param->getName(30), nullptr);

    return parameterTree;
}

/*ValueTree EffectTree::getTree(EffectTreeBase *effect) {
    return effectTree.getChildWithProperty(IDs::component, effect);
}*/

ValueTree EffectTree::getTree(GuiObject* component) {
    if (effectTree.getProperty(IDs::component) == component) {
        return effectTree;
    }

    if (dynamic_cast<EffectTreeBase*>(component)) {
        return effectTree.getChildWithProperty(IDs::component, component);
    } else if (dynamic_cast<ConnectionLine*>(component)) {
        return effectTree.getChildWithProperty(IDs::component, component);
    } else if (dynamic_cast<Parameter*>(component)) {
        return effectTree.getChildWithProperty(IDs::component, component);
    }

    return getTree(effectTree, component);


    return ValueTree();
}

ValueTree EffectTree::getTree(ValueTree parent, GuiObject* component) {
    //Todo general "component" property
    auto childFound = parent.getChildWithProperty(IDs::component, component);

    if (childFound.isValid()) {
        return childFound;
    } else {
        if (auto p = component->getParentComponent()) {
            if (auto parentComponent = dynamic_cast<GuiObject *>(p)) {
                auto parentTree = getTree(parent, parentComponent);
                if (parentTree.isValid()) {
                    return parentTree.getChildWithProperty(IDs::component, component);
                }
            }
        }
    }

    return ValueTree();
}

EffectTree::~EffectTree() {
    // Remove references to RefCountedObjects
    effectsToDelete.clear(); // effect references stored in toDelete array

    auto effectScene = effectTree.getProperty(IDs::component).getObject();
    effectScene->incReferenceCount();

    effectTree = ValueTree(); // clear ValueTree

    effectScene->decReferenceCountWithoutDeleting();
}



void EffectTree::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    // ADD EFFECT
    if (childWhichHasBeenAdded.hasType(EFFECT_ID)) {
        if (childWhichHasBeenAdded.hasProperty(Effect::IDs::initialised)) {
            if (auto e = getFromTree<Effect>(childWhichHasBeenAdded)) {
                e->setVisible(true);

                // Remove from effectsToDelete
                if (effectsToDelete.contains(e)) {
                    effectsToDelete.removeObject(e);
                }

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
            /*childWhichHasBeenAdded.setProperty(Effect::IDs::initialised, true, nullptr);
            auto parent = getFromTree<EffectTreeBase>(parentTree);
            auto effect = getFromTree<Effect>(effectTree);
            parent->addAndMakeVisible(effect);*/
        }
    }
        // ADD CONNECTION
    else if (childWhichHasBeenAdded.hasType(CONNECTION_ID)) {
        ConnectionLine *line;
        // Add connection here
        if (!childWhichHasBeenAdded.hasProperty(IDs::component)) {
            auto inport = getPropertyFromTree<ConnectionPort>(childWhichHasBeenAdded, ConnectionLine::IDs::InPort);
            auto outport = getPropertyFromTree<ConnectionPort>(childWhichHasBeenAdded, ConnectionLine::IDs::OutPort);

            line = new ConnectionLine();
            line->setInPort(inport);
            line->setOutPort(outport);

            childWhichHasBeenAdded.setProperty(IDs::component, line, nullptr);

            auto parent = getFromTree<EffectTreeBase>(parentTree);
            parent->addAndMakeVisible(line);
        } else {
            line = getPropertyFromTree<ConnectionLine>(childWhichHasBeenAdded,
                                                       IDs::component);
            line->setVisible(true);
        }
        line->toFront(false);

        line->connect();
    }
        // PARAMETER
    else if (childWhichHasBeenAdded.hasType(PARAMETER_ID)) {
        auto parameter = getFromTree<Parameter>(childWhichHasBeenAdded);
        if (parameter != nullptr) {
            parameter->setVisible(true);
        }
    }
    Listener::valueTreeChildAdded(parentTree, childWhichHasBeenAdded);
}

void EffectTree::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
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
                        auto connection = getPropertyFromTree<ConnectionLine>(child, IDs::component);

                        if (e->hasConnection(connection)) {
                            parentTree.removeChild(child, undoManager);
                            childrenRemoved++;
                        }
                    }
                }

                parent->removeChildComponent(e);
                e->setVisible(false);
                effectsToDelete.add(e);
            }
        }
    }
        // CONNECTION
    else if (childWhichHasBeenRemoved.hasType(CONNECTION_ID)) {
        auto line = getPropertyFromTree<ConnectionLine>(childWhichHasBeenRemoved, IDs::component);
        line->setVisible(false);

        EffectTreeBase::disconnectAudio(*line);
    }
        // PARAMETER
    else if (childWhichHasBeenRemoved.hasType(PARAMETER_ID)) {
        auto parameter = getFromTree<Parameter>(childWhichHasBeenRemoved);
        parameter->setVisible(false);

        // REMOVE PARAMETER CONNECTION
        //todo
        // if parameter has external connection connected then remove connection from parentTree
        /*if (parameter->isConnected()) {
            for (int i = 0; i < parentTree.getNumChildren(); i++) {
                auto child = parentTree.getChild(i);
                if (child.hasType(CONNECTION_ID)) {
                    auto connection = getFromTree<ConnectionLine>(child);
                    if (connection->getInPort() == parameter->getPort(false)
                        || connection->getOutPort() == parameter->getPort(false))
                    {
                        child.getParent().removeChild(child, &undoManager);
                    }
                }
            }
        }*/

        //todo
        // if parameter has internal connection connected then remove connection from parentTree parent

    }
    Listener::valueTreeChildRemoved(parentTree, childWhichHasBeenRemoved, indexFromWhichChildWasRemoved);
}



void EffectTree::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    auto component = getFromTree<Component>(treeWhosePropertyHasChanged);

    // Position
    if (property == Identifier("x")) {
        int newX = treeWhosePropertyHasChanged.getProperty("x");
        component->setTopLeftPosition(newX, component->getY());
    } else if (property == Identifier("y")) {
        int newY = treeWhosePropertyHasChanged.getProperty("y");
        component->setTopLeftPosition(component->getX(), newY);
    }

    Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}



void EffectTree::loadUserState() {
    if (getAppProperties().getUserSettings()->getValue(KEYNAME_LOADED_EFFECTS).isNotEmpty()) {
        auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);
        if (loadedEffectsData != nullptr) {
            ValueTree effectLoadDataTree = ValueTree::fromXml(*loadedEffectsData);

            std::cout << effectLoadDataTree.toXmlString() << newLine;

            loadEffect(effectTree, effectLoadDataTree);
        }
    }
}

void EffectTree::storeAll() {
    auto savedState = storeEffect(effectTree).createXml();

    std::cout << "Save state: " << savedState->toString() << newLine;
    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, savedState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();
}

EffectTreeBase::EffectTreeBase() {
    //dragLine.setAlwaysOnTop(true);
}


ValueTree EffectTree::storeEffect(const ValueTree &tree) {
    ValueTree copy(tree.getType());
    copy.copyPropertiesFrom(tree, nullptr);

    std::cout << "Storing: " << tree.getType().toString() << newLine;

    if (tree.hasType(EFFECT_ID)) {
        // Remove unused properties
        copy.removeProperty(Effect::IDs::initialised, nullptr);
        copy.removeProperty(Effect::IDs::connections, nullptr);

        // Set properties based on object
        if (auto effect = dynamic_cast<Effect*>(tree.getProperty(
                IDs::component).getObject())) {

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
        copy.removeProperty(IDs::component, nullptr);

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
                if (childParamObject->getParameter() != nullptr) {
                    childParam.setProperty("value", childParamObject->getParameter()->getValue(), nullptr);
                }

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

void EffectTree::loadEffect(ValueTree &parentTree, const ValueTree &loadData) {
    ValueTree copy(loadData.getType());

    //auto undoManagerToUse = appState == loading ? nullptr : &undoManager;

    // Effectscene added first
    if (loadData.hasType(EFFECTSCENE_ID)) {
        // Set children of Effectscene, and object property
        auto effectSceneObject = parentTree.getProperty(IDs::component);

        copy.setProperty(IDs::component, effectSceneObject, nullptr);
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

        parentTree.appendChild(copy, nullptr);

        // Load child effects
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);

            if (child.hasType(EFFECT_ID)) {
                loadEffect(copy, child);
            }
        }
    }

    // Add parameter connections
    if (copy.hasType(EFFECT_ID)) {
        for (int i = 0; i < copy.getNumChildren(); i++) {
            auto child = copy.getChild(i);

            // Connect parameters

            if (child.hasType(PARAMETER_ID)) {
                if (child.hasProperty("connectedParam")) {
                    auto connectedParamName = child.getProperty("connectedParam");

                    for (int j = 0; j < copy.getNumChildren(); j++) {
                        // Check all children of currently loading effect for a matching parameter child
                        auto c = copy.getChild(j).getChildWithProperty("name", connectedParamName);

                        if (c.isValid()) {
                            auto thisParam = getFromTree<Parameter>(child);
                            auto toConnectParam = getFromTree<Parameter>(c);
                            thisParam->connect(toConnectParam);
                            auto newConnection = new ConnectionLine();

                            newConnection->setInPort(thisParam->getPort(false));
                            newConnection->setOutPort(toConnectParam->getPort(true));

                            getFromTree<Effect>(copy)->addAndMakeVisible(newConnection);
                        }
                    }
                }
            }
        }
    }


    // Add Connections
    if (loadData.hasType(EFFECT_ID) || loadData.hasType(EFFECTSCENE_ID)) {
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);

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
    if (copy.getType() == EFFECT_ID) {
        loadEffect(copy);
    }
}

//todo just use existing parameterData parent instead of effect
Parameter::Ptr EffectTree::loadParameter(Effect* effect, ValueTree parameterData) {
    auto name = parameterData.getProperty("name");

    AudioProcessorParameter* param = nullptr;

    if (effect->isIndividual()) {
        for(auto p : effect->getParameters(false)) {
            if (p->getName(30).compare(name) == 0) {
                param = p;
            }
        }
    } else {
        param = new MetaParameter(name);
        EffectTreeBase::getAudioGraph()->addParameter(param);
    }

    Parameter::Ptr parameter = new Parameter(param);
    //effect->parameterArray.add(parameter);

    if (effect->isIndividual()) {
        parameter->setEditMode(false);
    } else {
        parameter->setEditMode(effect->isInEditMode());
    }

    parameterData.setProperty(IDs::component, parameter.get(), nullptr);

    effect->addAndMakeVisible(parameter.get());
    effect->addAndMakeVisible(parameter->getPort(true));

    // Set position

    int x = parameterData.getProperty("x");
    int y = parameterData.getProperty("y");

    // Set value
    if (parameterData.hasProperty("value")) {
        float value = parameterData.getProperty("value");
        param->setValueNotifyingHost(value);
    }

    parameter->setTopLeftPosition(x, y);


    // if has connection then connect


    return parameter;
}



template<class T>
T *EffectTree::getFromTree(const ValueTree &vt) {
    if (vt.hasProperty(IDs::component)) {
        // Return effect
        return dynamic_cast<T*>(vt.getProperty(IDs::component).getObject());
    } else if (vt.hasProperty(IDs::component)) {
        // Return connection
        return dynamic_cast<T*>(vt.getProperty(IDs::component).getObject());
    } else if (vt.hasProperty(IDs::component)) {
        // Return parameter
        return dynamic_cast<T*>(vt.getProperty(IDs::component).getObject());
    }
}

template<class T>
T *EffectTree::getPropertyFromTree(const ValueTree &vt, Identifier property) {
    return dynamic_cast<T*>(vt.getProperty(property).getObject());
}

void EffectTree::remove(SelectHoverObject *c) {
    if (auto l = dynamic_cast<ConnectionLine*>(c)) {
        auto parentTree = getTree(dynamic_cast<EffectTreeBase*>(l->getParentComponent()));
        auto lineTree = parentTree.getChildWithProperty(IDs::component, l);
        parentTree.removeChild(lineTree, undoManager);
    } else if (auto e = dynamic_cast<Effect*>(c)) {
        auto effectTree = getTree(e);
        effectTree.getParent().removeChild(effectTree, undoManager);
    } else if (auto p = dynamic_cast<Parameter*>(c)) {
        auto parentTree = getTree(dynamic_cast<Effect*>(p->getParentComponent()));
        auto paramTree = parentTree.getChildWithProperty(IDs::component, p);
        parentTree.removeChild(paramTree, undoManager);
    }
}

