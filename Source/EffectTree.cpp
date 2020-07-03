/*
  ==============================================================================

    EffectTree.cpp
    Created: 8 May 2020 9:42:07am
    Author:  maxime

  ==============================================================================
*/

#include "EffectTree.h"
#include "Audio-Effects/Chorus.h"
#include "Audio-Effects/Delay.h"
#include "Audio-Effects/CompressorExpander.h"
#include "Audio-Effects/ChannelSplitter.h"
#include "Audio-Effects/Distortion.h"
#include "Audio-Effects/Flanger.h"
#include "Audio-Effects/Gain.h"
#include "Audio-Effects/Panning.h"
#include "Audio-Effects/ParametricEQ.h"
#include "Audio-Effects/Phaser.h"
#include "Audio-Effects/PitchShift.h"
#include "Audio-Effects/Reverb.h"
#include "Audio-Effects/RingModulation.h"
#include "Audio-Effects/RobotizationWhisperization.h"
#include "Audio-Effects/Level.h"
#include "Audio-Effects/Tremolo.h"
#include "Audio-Effects/WahWah.h"
#include "Audio-Effects/IOEffects.h"
#include "Audio-Effects/Vibrato.h"
#include "Audio-Effects/Oscillator.h"

Identifier EffectTree::IDs::component = "component";
Identifier PORT_ID = "port";


EffectTree::EffectTree(EffectTreeBase* effectScene)
    : effectTree(EFFECTSCENE_ID)
    , undoManager(&effectScene->getUndoManager())
{
    setupProcessors();

    effectTree.setProperty(IDs::component, effectScene, nullptr);
    effectTree.addListener(this);
    effectScene->addComponentListener(this);
}


Effect* EffectTree::createEffect(ValueTree tree) {
    if (! tree.getParent().isValid()) {
        effectTree.appendChild(tree, undoManager);
    }

    auto effect = new Effect();
    effect->state = Effect::loading;
    tree.setProperty(IDs::component, effect, nullptr);
    effect->addComponentListener(this);


    int id = tree.getProperty(Effect::IDs::processorID);
    if (! tree.hasProperty(Effect::IDs::processorID)) {
        id = 0;
    }

    if (id > 0) {
        // Individual Effect
        std::unique_ptr<AudioProcessor> newProcessor;

        newProcessor = createProcessor(id);

        auto node = EffectTreeBase::getAudioGraph()->addNode(move(newProcessor));
        effect->setNode(node);
        // Create from node:
        effect->setProcessor(node->getProcessor());
    }

    //======================================================================
    // Set up Ports
    if (! tree.getChildWithName(PORT_ID).isValid() && effect->isIndividual()) {
        // Set up ports based on processor buses
        int numInputBuses = effect->getProcessor()->getBusCount(true);
        int numBuses = numInputBuses + effect->getProcessor()->getBusCount(false);
        for (int i = 0; i < numBuses; i++) {
            bool isInput = i < numInputBuses;
            // Get bus from processor
            auto bus = isInput ? effect->getProcessor()->getBus(true, i) :
                       effect->getProcessor()->getBus(false, i - numInputBuses);
            // Check channel number - if 0 ignore
            if (bus->getNumberOfChannels() == 0)
                continue;

            // Create port
            effect->addPort(bus, isInput);
        }
    } else {
        // Load ports from valueTree
        for (int i = 0; i < tree.getNumChildren(); i++) {
            auto child = tree.getChild(i);
            if (child.hasType(PORT_ID)) {
                loadPort(child);
            }
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
                ValueTree parameterTree(PARAMETER_ID);
                parameterTree.setProperty("name", processorParam->getName(30), nullptr);

                // Set position
                auto x = (effect->getNumInputs()) > 0 ? 60 : 30;
                auto y = 30 + i * 50;

                parameterTree.setProperty("x", x, nullptr);
                parameterTree.setProperty("y", y, nullptr);

                // Add parameterVT to tree
                parameterChildren.add(parameterTree);
                tree.appendChild(parameterTree, nullptr);
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


    //==============================================================
    // Set up Menu
    effect->setupMenu();

    // Set up Title
    effect->setupTitle();

    // Set new bounds
    effect->setBounds(x, y, w, h);
    //effect->setTopLeftPosition(x,y);

    if (effect->isInEditMode()) {
        effect->expandToFitChildren();
    }

    effect->resized();

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

    return newEffect;
}


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
            if (! parameter->isInEditMode()) {
                auto effect = dynamic_cast<Effect *>(parameter->getParentComponent());
                jassert(effect != nullptr);

                auto parameterTree = getTree(effect).getChildWithProperty(IDs::component, parameter);

                if (wasMoved) {
                    parameterTree.setProperty("x", component.getX(), undoManager);
                    parameterTree.setProperty("y", component.getY(), undoManager);
                }
            }
        }
    }
    ComponentListener::componentMovedOrResized(component, wasMoved, wasResized);
}

void EffectTree::componentNameChanged(Component &component) {
    if (auto effect = dynamic_cast<Effect*>(&component)) {
        auto effectTree = getTree(effect);
        if (effectTree.isValid()) {
            effect->incReferenceCount();
            effectTree.setProperty(Effect::IDs::name, component.getName(), undoManager);
            effect->decReferenceCountWithoutDeleting();
        }
    }

    ComponentListener::componentNameChanged(component);
}

void EffectTree::componentChildrenChanged(Component &component) {
    // Effect or EffectScene
    if (auto effectTreeBase = dynamic_cast<EffectTreeBase*>(&component)) {

        auto effectTree = getTree(effectTreeBase);
        if (effectTree.isValid()) {

            for (auto c : component.getChildren()) {

                //todo figure out how to better control these listeners
                c->addComponentListener(this);

                auto child = dynamic_cast<SelectHoverObject *>(c);

                if (child != nullptr) {
                    ValueTree childTree;
                    for (int i = 0; i < effectTree.getNumChildren(); i++) {
                        if (effectTree.getChild(i).getProperty(IDs::component).getObject() == child) {
                            childTree = effectTree.getChild(i);
                        }
                    }

                    if (childTree.isValid()) {

                    } else {
                        // Create ConnectionLine tree
                        if (auto line = dynamic_cast<ConnectionLine *>(c)) {
                            ValueTree newLineTree(CONNECTION_ID);
                            newLineTree.setProperty(IDs::component, line, nullptr);
                            effectTree.appendChild(newLineTree, undoManager);
                        }
                            // Create Parameter Tree
                        else if (auto parameter = dynamic_cast<Parameter *>(c)) {
                            ValueTree newParameterTree(PARAMETER_ID);
                            newParameterTree.setProperty(IDs::component, parameter, nullptr);
                            effectTree.appendChild(newParameterTree, undoManager);
                        }
                        // Reassign Effect
                        else if (auto effect = dynamic_cast<Effect *>(c)) {
                            if (! childTree.isValid()) {
                                childTree = getTree(effect);
                            }
                            if (! childTree.isValid()) {
                                childTree = findTree(effectTree, effect);
                            }

                            auto newChildParent = getTree(
                                    dynamic_cast<SelectHoverObject *>(effect->getParentComponent()));
                            if (childTree.getParent() != newChildParent) {
                                // All we're doing here is updating the trees to match components
                                childTree.getParent().removeChild(childTree, undoManager);
                                newChildParent.appendChild(childTree, undoManager);
                            }
                        }
                        // Create Port
                        else if (auto port = dynamic_cast<ConnectionPort*>(c)) {
                            auto portTree = ValueTree(PORT_ID);
                            portTree.setProperty(IDs::component, port, nullptr);
                            String portID = std::to_string(reinterpret_cast<int64>(port));
                            //port->setComponentID(portID);

                            effectTree.appendChild(portTree, undoManager);
                        }
                    }
                }
            }
        }
    }


    ComponentListener::componentChildrenChanged(component);
}



void EffectTree::componentParentHierarchyChanged(Component &component) {
    //todo delete connectionLine tree when removed from parent
    if (auto line = dynamic_cast<ConnectionLine*>(&component)) {
        if (line->getParentComponent() == nullptr) {
            auto tree = findTree(effectTree,line);
            if (tree.isValid()) {
                tree.getParent().removeChild(tree, undoManager);
            }
        }
    } else if (auto port = dynamic_cast<ConnectionPort*>(&component)) {
        if (port->getParentComponent() == nullptr) {
            auto tree = findTree(effectTree, port);
            if (tree.isValid()) {
                tree.getParent().removeChild(tree, undoManager);
            }
        }
    }
    ComponentListener::componentParentHierarchyChanged(component);
}


void EffectTree::componentEnablementChanged(Component &component) {
    if (! undoManager->isPerformingUndoRedo()) {
        if (auto line = dynamic_cast<ConnectionLine *>(&component)) {
            if (line->isEnabled()) {
                // Line is connected
                auto lineTree = getTree(line);
                jassert(lineTree.isValid());

                // Update tree
                lineTree.setProperty(ConnectionLine::IDs::InPort, line->getInPort().get(), undoManager);
                lineTree.setProperty(ConnectionLine::IDs::OutPort, line->getOutPort().get(), undoManager);

            } else {
                // Line is disconnected
                auto lineTree = getTree(line);
                if (lineTree.isValid()) {
                    auto inPort = line->getInPort().get();
                    if (inPort == nullptr) {
                        lineTree.removeProperty(ConnectionLine::IDs::InPort, undoManager);
                    }
                    auto outPort = line->getOutPort().get();
                    if (outPort == nullptr) {
                        lineTree.removeProperty(ConnectionLine::IDs::OutPort, undoManager);
                    }
                }
            }
        }
    }
    ComponentListener::componentEnablementChanged(component);
}


void EffectTree::componentBeingDeleted(Component &component) {
    component.removeComponentListener(this);

    for (auto child : component.getChildren()) {
        child->removeComponentListener(this);
    }

    ComponentListener::componentBeingDeleted(component);
}

/**
 * Finds tree based on component hierarchy - meant to be efficient.
 * @param component
 * @return
 */
ValueTree EffectTree::getTree(GuiObject* component) {
    if (effectTree.getProperty(IDs::component) == component) {
        return effectTree;
    }

    auto childFound = effectTree.getChildWithProperty(IDs::component, component);

    if (childFound.isValid()) {
        return childFound;
    } else {
        if (auto p = component->getParentComponent()) {
            if (auto parentComponent = dynamic_cast<GuiObject *>(p)) {
                auto parentTree = getTree(parentComponent);
                if (parentTree.isValid()) {
                    return parentTree.getChildWithProperty(IDs::component, component);
                }
            }
        }
    }

    return ValueTree();
}


EffectTree::~EffectTree() {
    auto effectScene = effectTree.getProperty(IDs::component).getObject();
    effectScene->incReferenceCount();

    removeAllListeners();

    // Clear ValueTree
    effectTree = ValueTree();

    effectScene->decReferenceCountWithoutDeleting();
}



void EffectTree::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) {
    if (childWhichHasBeenAdded.hasProperty(IDs::component)) {
        auto component = getFromTree<Component>(childWhichHasBeenAdded);
        auto parent = getFromTree<EffectTreeBase>(parentTree);

        component->addComponentListener(this);

        // Type-specific operations
        if (auto line = dynamic_cast<ConnectionLine *>(component)) {
            line->connect();
        }
        if (childWhichHasBeenAdded.hasType(PARAMETER_ID)) {
            std::cout << "add parameter" << newLine;
        }


        if (parent != nullptr && parent != component->getParentComponent()) {
            parent->addAndMakeVisible(component);
        }
    }

    Listener::valueTreeChildAdded(parentTree, childWhichHasBeenAdded);
}

void EffectTree::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                           int indexFromWhichChildWasRemoved) {


    if (childWhichHasBeenRemoved.hasProperty(IDs::component)) {
        auto component = getFromTree<Component>(childWhichHasBeenRemoved);
        auto parent = component->getParentComponent();

        //component->removeComponentListener(this);

        auto parentFromTree = getFromTree<Component>(parentTree);
        // Type-specific operations
        if (auto line = dynamic_cast<ConnectionLine*>(component)) {
            line->disconnect();
        }
        else if (auto effect = dynamic_cast<Effect*>(component)) {
            if (! undoManager->isPerformingUndoRedo()) {
                for (auto c : effect->getConnectionsToThis()) {
                    auto connectionTree = getTree(c);
                    connectionTree.getParent().removeChild(connectionTree, undoManager);
                }
            }
        }

        if (parent != nullptr && parent == parentFromTree) {
            parent->removeChildComponent(component);
        }

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
    } else if (property == Identifier("w")) {
        int newW = treeWhosePropertyHasChanged.getProperty("w");
        component->setSize(newW, component->getHeight());
    } else if (property == Identifier("h")) {
        int newH = treeWhosePropertyHasChanged.getProperty("h");
        component->setSize(component->getWidth(), newH);
    }

    if (treeWhosePropertyHasChanged.hasType(CONNECTION_ID)) {
        auto line = getFromTree<ConnectionLine>(treeWhosePropertyHasChanged);
        if (property == Identifier(ConnectionLine::IDs::InPort)) {
            // Set line inPort
            auto inPort = getPropertyFromTree<ConnectionPort>(treeWhosePropertyHasChanged, ConnectionLine::IDs::InPort);
            if (inPort != nullptr) {
                line->setPort(inPort);
            } else if (line->isConnected()) {
                line->unsetPort(line->getInPort().get());
            }
        } else if (property == Identifier(ConnectionLine::IDs::OutPort)) {
            auto outPort = getPropertyFromTree<ConnectionPort>(treeWhosePropertyHasChanged, ConnectionLine::IDs::OutPort);
            if (outPort != nullptr) {
                line->setPort(outPort);
            } else if (line->isConnected()){
                line->unsetPort(line->getOutPort().get());
            }
        }
    }

    Listener::valueTreePropertyChanged(treeWhosePropertyHasChanged, property);
}



void EffectTree::loadTemplate(String name) {
    if (name.isEmpty()) {
        // Load user state
        name = "default";
    }

    auto effectLoadDataTree = EffectLoader::loadTemplate(name);
    currentTemplateName = name;

    if (effectLoadDataTree.isValid()) {
        loadEffect(effectTree, effectLoadDataTree);
    } else {
        std::cout << "empty template." << newLine;
    }



    /*if (getAppProperties().getUserSettings()->getValue(KEYNAME_LOADED_EFFECTS).isNotEmpty()) {
        auto loadedEffectsData = getAppProperties().getUserSettings()->getXmlValue(KEYNAME_LOADED_EFFECTS);
        if (loadedEffectsData != nullptr) {
            ValueTree effectLoadDataTree = ValueTree::fromXml(*loadedEffectsData);

            std::cout << effectLoadDataTree.toXmlString() << newLine;

            loadEffect(effectTree, effectLoadDataTree);
        }
    }*/
}

void EffectTree::storeTemplate(String name) {
    auto saveState = storeEffect(effectTree);

    saveState.setProperty("name", name, nullptr);
    EffectLoader::saveTemplate(saveState);

/*    auto savedState = storeEffect(effectTree).createXml();

    std::cout << "Save state: " << savedState->toString() << newLine;
    getAppProperties().getUserSettings()->setValue(KEYNAME_LOADED_EFFECTS, savedState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();*/
}

void EffectTree::clear() {
    if (effectTree.getNumChildren() > 0) {
        effectTree.removeAllChildren(undoManager);
        //effectTree.removeAllProperties(undoManager);
    }
}



ValueTree EffectTree::storeEffect(const ValueTree &storeData) {
    ValueTree copy(storeData.getType());
    copy.copyPropertiesFrom(storeData, nullptr);

    std::cout << "Storing: " << storeData.getType().toString() << newLine;

    if (storeData.hasType(EFFECT_ID)) {
        // Remove unused properties
        copy.removeProperty(Effect::IDs::initialised, nullptr);
        copy.removeProperty(Effect::IDs::connections, nullptr);

        // Set properties based on object
        if (auto effect = dynamic_cast<Effect*>(storeData.getProperty(
                IDs::component).getObject())) {

            // Set size property
            copy.setProperty(Effect::IDs::w, effect->getWidth(), nullptr);
            std::cout << effect->getWidth() << " " << effect->getHeight() << newLine;
            copy.setProperty(Effect::IDs::h, effect->getHeight(), nullptr);

            // Set ID property (for port connections)
            copy.setProperty("ID", reinterpret_cast<int64>(effect), nullptr);

            // Set num ports
            /*copy.setProperty("numInputPorts", effect->getNumInputs(), nullptr);
            copy.setProperty("numOutputPorts", effect->getNumOutputs(), nullptr);*/

            // Set edit mode
            copy.setProperty("editMode", effect->isInEditMode(), nullptr);

        } else {
            std::cout << "dat shit is not initialised. do not store" << newLine;
            return ValueTree();
        }
    }

    if (storeData.hasType(EFFECTSCENE_ID) || storeData.hasType(EFFECT_ID)) {
        copy.removeProperty(IDs::component, nullptr);

        for (int i = 0; i < storeData.getNumChildren(); i++) {
            auto child = storeData.getChild(i);

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

                if (dynamic_cast<SliderParameter*>(childParamObject) != nullptr) {
                    childParam.setProperty("type", 2, nullptr); /// BUTTON = 0, COMBO = 1, SLIDER = 2
                } else if (dynamic_cast<ComboParameter*>(childParamObject) != nullptr) {
                    childParam.setProperty("type", 1, nullptr);
                } else if (dynamic_cast<ButtonParameter*>(childParamObject) != nullptr) {
                    childParam.setProperty("type", 0, nullptr);
                }

                if (childParamObject->getParameter() != nullptr) {
                    if (dynamic_cast<ComboParameter*>(childParamObject) != nullptr) {
                        auto param = dynamic_cast<AudioParameterChoice*>(childParamObject->getParameter());
                        auto value = param->getIndex();
                                            std::cout << "Storing parameter: " << childParamObject->getName() << " value: " << value << newLine;
                        childParam.setProperty("value", value, nullptr);
                    } else {
                        auto value = childParamObject->getParameter()->getValue();
                    std::cout << "Storing parameter: " << childParamObject->getName() << " value: " << value << newLine;
                    childParam.setProperty("value", value, nullptr);
                    }
                }

                childParam.setProperty("internalPortID",
                        reinterpret_cast<int64>(childParamObject->getPort(true)), nullptr);
                childParam.setProperty("externalPortID",
                                       reinterpret_cast<int64>(childParamObject->getPort(false)), nullptr);

                /*if (childParamObject->connected()) {
                    childParam.setProperty("connectedParam",
                                           childParamObject->getConnectedParameter()->getName(), nullptr);
                }*/

                copy.appendChild(childParam, nullptr);
            }
            // Register connection
            else if  (child.hasType(CONNECTION_ID))
            {
                auto line = getFromTree<ConnectionLine>(child);
                if (line == nullptr || ! line->isEnabled()) {
                    // Invalid line, ignore
                    //jassertfalse;
                    continue;
                }

                // Get data to set
                auto inPort = line->getInPort();
                auto outPort = line->getOutPort();

                /*auto effect1 = dynamic_cast<Effect*>(inPortObject->getParentComponent());
                auto effect2 = dynamic_cast<Effect*>(outPortObject->getParentComponent());*/

                /*auto inPortID = effect1->getPortID(inPortObject);
                auto outPortID = effect2->getPortID(outPortObject);*/

                // Set data

                ValueTree connectionToSet(CONNECTION_ID);

                /*connectionToSet.setProperty("inPortEffect", reinterpret_cast<int64>(effect1), nullptr);
                connectionToSet.setProperty("outPortEffect", reinterpret_cast<int64>(effect2), nullptr);*/

                connectionToSet.setProperty("inPortID", reinterpret_cast<int64>(inPort.get()), nullptr);
                connectionToSet.setProperty("outPortID", reinterpret_cast<int64>(outPort.get()), nullptr);

                /*connectionToSet.setProperty("inPortIsInternal",
                                            (dynamic_cast<InternalConnectionPort*>(inPortObject) != nullptr), nullptr);
                connectionToSet.setProperty("outPortIsInternal",
                                            (dynamic_cast<InternalConnectionPort*>(outPortObject) != nullptr), nullptr);*/

                // Set data to ValueTree
                copy.appendChild(connectionToSet, nullptr);
            }
            // Port
            else if (child.hasType(PORT_ID)) {
                auto childCopy = child.createCopy();
                auto port = getFromTree<ConnectionPort>(child);

                if (port != nullptr) {
                    childCopy.removeProperty(IDs::component, nullptr);
                    childCopy.setProperty("ID", reinterpret_cast<int64>(port), nullptr);
                    childCopy.setProperty("isInput", port->isInput, nullptr);
                    childCopy.setProperty("isInternal", port->isInternal, nullptr);

                    childCopy.setProperty("linkedPortID", reinterpret_cast<int64>(port->getLinkedPort()), nullptr);

                    if (dynamic_cast<AudioPort *>(port) || dynamic_cast<InternalConnectionPort *>(port)) {
                        childCopy.setProperty("type", "audio", nullptr);
                    } else if (dynamic_cast<ParameterPort *>(port)) {
                        childCopy.setProperty("type", "parameter", nullptr);
                    }

                    copy.appendChild(childCopy, nullptr);
                }
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
        copy.copyPropertiesAndChildrenFrom(parentTree, nullptr);

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

        // Load Effect Child Trees needed in Effect construction
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i);
            if (child.hasType(PARAMETER_ID)) {
                ValueTree paramChild(PARAMETER_ID);
                paramChild.copyPropertiesFrom(child, nullptr);

                copy.appendChild(paramChild, nullptr);
            } else if (child.hasType(PORT_ID)) {
                ValueTree portChild(PORT_ID);
                portChild.copyPropertiesFrom(child, nullptr);

                copy.appendChild(portChild, nullptr);
            }
        }

        // Create effect
        parentTree.appendChild(copy, nullptr);
        createEffect(copy);

        // Load child effects
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            auto child = loadData.getChild(i).createCopy();

            if (child.hasType(EFFECT_ID)) {
                copy.appendChild(child, nullptr);
                loadEffect(copy, child);
            }
        }

    }

    // Add Connections
    if (loadData.hasType(EFFECT_ID) || loadData.hasType(EFFECTSCENE_ID)) {
        for (int i = 0; i < loadData.getNumChildren(); i++) {
            // Connection
            if (loadData.getChild(i).hasType(CONNECTION_ID)) {
                auto child = loadData.getChild(i).createCopy();
                copy.appendChild(child, nullptr);
                // Set data
                loadConnection(child);
            }
        }
    }

    // Set Effect state to neutral
    auto effect = getFromTree<EffectTreeBase>(copy);
    effect->state = Effect::neutral;
}

//todo just use existing parameterData parent instead of effect
Parameter::Ptr EffectTree::loadParameter(Effect* effect, ValueTree parameterData) {
    String name = parameterData.getProperty("name");

    Parameter::Ptr parameter = nullptr;
    AudioProcessorParameter* param = nullptr;
    if (effect->isIndividual()) {
        for(auto p : effect->getParameters(false)) {
            if (p->getName(30).compare(name) == 0) {
                param = p;
            }
        }

        jassert(param != nullptr);
    }

    int type = parameterData.getProperty("type", -1);
    if (type == -1 && param != nullptr) {
        if (dynamic_cast<AudioParameterBool*>(param) != nullptr) {
            type = 0;
        } else if (dynamic_cast<AudioParameterChoice*>(param) != nullptr) {
            type = 1;
        } else if (dynamic_cast<AudioParameterFloat*>(param) != nullptr) {
            type = 2;
        }
    }
    jassert (type != -1);


    float value = parameterData.getProperty("value", param != nullptr ? param->getValue() : 0);

    if (type == 0) {
        parameter = new ButtonParameter(param);
    } else if (type == 1) {
        parameter = new ComboParameter(param);
    } else if (type == 2) {
        parameter = new SliderParameter(param);
    }

    parameter->setParentEditMode(effect->isInEditMode());

    if (parameter->getName() == "Parameter" && name != "") {
        parameter->setName(name);
    }

    if (parameterData.hasProperty("internalPortID")) {
        String portID = parameterData.getProperty("internalPortID");
        parameter->getPort(true)->setComponentID(portID);
    }
    if (parameterData.hasProperty("externalPortID")) {
        String portID = parameterData.getProperty("externalPortID");
        parameter->getPort(false)->setComponentID(portID);
    }

    //effect->parameterArray.add(parameter);

    if (effect->isIndividual()) {
        parameter->setEditMode(false);
    } else {
        parameter->setEditMode(effect->isInEditMode());
    }

    if (param != nullptr) { //todo for loading from tree
        if (auto baseEffect = dynamic_cast<BaseEffect*>(effect->getProcessor())) {
            if (baseEffect->outputParameters.contains(param)) {
                parameter->setIsOutput(true);
            }
        }
    }

    // Set value

    if (param != nullptr && name != "Device") {
        parameter->setParameterValueAsync(value);
    }

    // Add ValueTree and Component to system

    parameterData.setProperty(IDs::component, parameter.get(), nullptr);

    effect->addAndMakeVisible(parameter.get());

    // Set position

    int x = parameterData.getProperty("x");
    int y = parameterData.getProperty("y");

    parameter->setTopLeftPosition(x, y);

    return parameter;
}

/**
 * Creates a ConnectionLine from ValueTree data
 * @param connectionData is loadable ConnectionLine data, including parent to load to
 * @return newly created line
 */
ConnectionLine::Ptr EffectTree::loadConnection(ValueTree connectionData) {

    if (connectionData.hasProperty("inPortID") && connectionData.hasProperty("outPortID")) {
        auto parentTree = connectionData.getParent();

        String inPortID = connectionData.getProperty("inPortID");
        String outPortID = connectionData.getProperty("outPortID");

        auto parent = getFromTree<EffectTreeBase>(parentTree);
        auto inPort = parent->getPortFromID(inPortID);
        auto outPort = parent->getPortFromID(outPortID);

        if (inPort == nullptr || outPort == nullptr) {
            jassertfalse;
            return nullptr;
        }

        auto line = new ConnectionLine();

        line->setPort(inPort);
        line->setPort(outPort);

        parent->addAndMakeVisible(line);

        auto lineTree = getTree(line);
        lineTree.setProperty(ConnectionLine::IDs::InPort, line->getInPort().get(), nullptr);
        lineTree.setProperty(ConnectionLine::IDs::OutPort, line->getOutPort().get(), nullptr);

        return line;
    }
    jassertfalse;
}



template<class T>
T *EffectTree::getFromTree(const ValueTree &vt) {
    if (vt.hasProperty(IDs::component)) {
        return dynamic_cast<T*>(vt.getProperty(IDs::component).getObject());
    } else {
        return nullptr;
    }
}

template<class T>
T *EffectTree::getPropertyFromTree(const ValueTree &vt, Identifier property) {
    return dynamic_cast<T*>(vt.getProperty(property).getObject());
}

void EffectTree::remove(SelectHoverObject *c) {
    if (c->getParentComponent() == nullptr) {
        return;
    }

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

void EffectTree::loadEffect(const ValueTree &loadData) {
    loadEffect(effectTree, loadData);
}

ConnectionPort::Ptr EffectTree::loadPort(ValueTree portData) {
    auto effect = getFromTree<Effect>(portData.getParent());
    String ID = portData.getProperty("ID");
    bool isInput = portData.getProperty("isInput");
    bool isInternal = portData.getProperty("isInternal");
    String linkedID = portData.getProperty("linkedPortID");

    ConnectionPort* port = nullptr;

    String type = portData.getProperty("type");
    if (type == "audio") {
        if (! isInternal) {
            auto newPort = new AudioPort(isInput);
            portData.setProperty(IDs::component, newPort, nullptr);
            newPort->setComponentID(ID);
            effect->addPort(newPort);

            newPort->bus = Effect::getDefaultBus();

            newPort->internalPort->setComponentID(linkedID);

            return newPort;
        }
    }

    return port;
}

ValueTree EffectTree::findTree(ValueTree treeToSearch, GuiObject *component) {
    auto childFound = treeToSearch.getChildWithProperty(IDs::component, component);
    if (childFound.isValid()) {
        return childFound;
    } else {
        // Search children
        for (int i = 0; i < treeToSearch.getNumChildren(); i++) {
            childFound = findTree(treeToSearch.getChild(i), component);
            if (childFound.isValid()) {
                return childFound;
            }
        }
    }
    return ValueTree();
}

bool EffectTree::isNotEmpty() {
    return effectTree.getNumChildren() > 0;
}

String EffectTree::getCurrentTemplateName() {
    return currentTemplateName;
}

void EffectTree::removeAllListeners(ValueTree component) {
    if (component.isValid()) {
        for (int i = 0; i < component.getNumChildren(); i++) {
            auto child = component.getChild(i);

            if (child.hasProperty(IDs::component)) {
                getFromTree<Component>(child)->removeComponentListener(this);
            }

            if (child.getNumChildren() > 0) {
                removeAllListeners(child);
            }
        }
    } else {
        removeAllListeners(effectTree);
    }
}

StringArray EffectTree::getProcessorNames() {
    return processorNames;
}

String EffectTree::getProcessorName(int processorID) {
    return processorNames[processorID];
}

std::unique_ptr<AudioProcessor> EffectTree::createProcessor(int processorID) {
    makeProcessorArray[processorID-1]();
    return std::move(newProcessor);
}

void EffectTree::setupProcessors() {
    processorNames = {
        "Empty Effect"
         , "Input Device"
         , "Output Device"
         , "Distortion"
         , "Delay"
         , "Reverb"
         , "Chorus"
         , "Compressor"
         , "Channel Splitter"
         , "Flanger"
         , "Gain"
         , "Panning"
         , "EQ"
         , "Phaser"
         , "RingModulation"
         , "PitchShift"
         , "Robotization"
         , "Level Detector"
         , "Tremolo"
         , "Wahwah"
         , "Vibrato"
         , "Parameter Oscillator"
    };

    makeProcessorArray.add([=] { newProcessor = std::make_unique<IOEffect>(true); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<IOEffect>(false); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<DistortionAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<DelayAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<ReverbEffect>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<ChorusAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<CompressorExpanderAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<ChannelSplitterProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<FlangerAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<GainAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<PanningAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<ParametricEQAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<PhaserAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<RingModulationAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<PitchShiftAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<RobotizationWhisperizationAudioProcessor>(); });
    makeProcessorArray.add( [=] { newProcessor = std::make_unique<LevelAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<TremoloAudioProcessor>(); });
    makeProcessorArray.add([=] { newProcessor = std::make_unique<WahWahAudioProcessor>(); });
    makeProcessorArray.add([=] {newProcessor = std::make_unique<VibratoAudioProcessor>(); });
    makeProcessorArray.add([=] {newProcessor = std::make_unique<Oscillator>(); });
}






