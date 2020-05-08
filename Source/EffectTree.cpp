/*
  ==============================================================================

    EffectTree.cpp
    Created: 8 May 2020 9:42:07am
    Author:  maxime

  ==============================================================================
*/

#include "EffectTree.h"


EffectTree::EffectTree() {
    //todo create effect scene?
}


Effect* EffectTree::loadEffect(ValueTree tree) {
    auto effect = new Effect();
    tree.setProperty(EffectTreeBase::IDs::effectTreeBase, effect, nullptr);
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
            auto parameter = effect->loadParameter(tree.getChild(i));
            tree.getChild(i).setProperty(Parameter::IDs::parameterObject, parameter.get(), nullptr);
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

ValueTree EffectTree::newConnection(ConnectionPort::Ptr port1, ConnectionPort::Ptr port2) {
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
}



void EffectTree::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    if (auto effect = dynamic_cast<Effect*>(&component)) {
        if (wasMoved) {
            std::cout << "Effect move update" << newLine;
            getTree(effect).setProperty(Effect::IDs::x, effect->getX(), undoManager);
            getTree(effect).setProperty(Effect::IDs::y, effect->getY(), undoManager);
        }
        else if (wasResized) {
            std::cout << "Effect size update" << newLine;
            getTree(effect).setProperty(Effect::IDs::w, effect->getWidth(), undoManager);
            getTree(effect).setProperty(Effect::IDs::h, effect->getHeight(), undoManager);
        }
    } else if (auto parameter = dynamic_cast<Parameter*>(&component)) {
        auto effect = dynamic_cast<Effect*>(parameter->getParentComponent());
        if (wasMoved) {
            std::cout << "Parameter move update" << newLine;
            auto parameterTree = getTree(effect).getChildWithProperty(Parameter::IDs::parameterObject, parameter);
            parameterTree.setProperty("x", component.getX(), nullptr);
            parameterTree.setProperty("y", component.getY(), nullptr);
        }
    }



    ComponentListener::componentMovedOrResized(component, wasMoved, wasResized);
}

void EffectTree::componentNameChanged(Component &component) {
    if (auto e = dynamic_cast<Effect*>(&component)) {
        getTree(e).setProperty(Effect::IDs::name, component.getName(), undoManager);
    }

    ComponentListener::componentNameChanged(component);
}

void EffectTree::setUndoManager(UndoManager &um) {
    undoManager = &um;
}

void EffectTree::componentChildrenChanged(Component &component) {
    for (auto c : component.getChildren()) {
        c->addComponentListener(this);
    }
    ComponentListener::componentChildrenChanged(component);
}

ValueTree EffectTree::getTree(EffectTreeBase *effect) {
    return effectTree.getChildWithProperty(EffectTreeBase::IDs::effectTreeBase, effect);
}

EffectTree::~EffectTree() {
    // Remove references to RefCountedObjects
    effectsToDelete.clear(); // effect references stored in toDelete array
    effectTree = ValueTree(); // clear ValueTree
}

void EffectTree::valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded)
{


    Listener::valueTreeChildAdded(parentTree, childWhichHasBeenAdded);
}

void EffectTree::valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                                       int indexFromWhichChildWasRemoved)
{


    Listener::valueTreeChildRemoved(parentTree, childWhichHasBeenRemoved, indexFromWhichChildWasRemoved);
}

