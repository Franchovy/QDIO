/*
  ==============================================================================

    Effect.h
    Created: 31 Mar 2020 9:03:16pm
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>
#include "EffectGui.h"

#pragma once

class Effect;

/**
    GuiEffect Component
    GUI Representation of Effects / Container for plugins

    v-- Is this necessary??
    ReferenceCountedObject for usage as part of ValueTree system
*/
class GuiEffect : public GuiObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GuiEffect>;

    GuiEffect (const MouseEvent &event, Effect* parentEVT);
    ~GuiEffect() override;

    void setProcessor(AudioProcessor* processor);

    void paint (Graphics& g) override;
    void resized() override;
    void moved() override;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    void childrenChanged() override;
    void parentHierarchyChanged() override;

    ConnectionPort::Ptr checkPort(Point<int> pos);

    void setParameters(const AudioProcessorParameterGroup* group);
    void addParameter(AudioProcessorParameter* param);

    void addPort(AudioProcessor::Bus* bus, bool isInput);

    Point<int> dragDetachFromParentComponent();
    bool isIndividual() const { return individual; }
    bool hasBeenInitialised = false;

    Effect* EVT;

    bool hoverMode = false;
    bool selectMode = false;

    void setEditMode(bool isEditMode);
    bool toggleEditMode() { setEditMode(!editMode); }
    bool isInEditMode() { return editMode; }

    CustomMenuItems& getMenu() { return editMode ? editMenu : menu; }

private:
    bool individual = false;
    bool editMode = false;
    OwnedArray<AudioPort> inputPorts;
    OwnedArray<AudioPort> outputPorts;
    Label title;
    Image image;

    Resizer resizer;
    ComponentDragger dragger;
    ComponentBoundsConstrainer constrainer;

    CustomMenuItems menu;
    CustomMenuItems editMenu;

    Component* currentParent = nullptr;

    const AudioProcessorParameterGroup* parameters;

    //============================================================================
    // GUI auto stuff

    int portIncrement = 50;
    int inputPortStartPos = 100;
    int inputPortPos = inputPortStartPos;
    int outputPortStartPos = 100;
    int outputPortPos = outputPortStartPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuiEffect)
};

/**
 * Base class for Effects and EffectScene
 * Contains all functionality common to both:
 * Convenience functions for tree navigation,
 * Data for saving and loading state,
 */
class EffectTreeBase : public ReferenceCountedObject, public Component, public ValueTree::Listener, public LassoSource<GuiObject::Ptr>
{
public:
    EffectTreeBase(Identifier id) : tree(id) {

    }

    void createConnection(std::unique_ptr<ConnectionLine> line);

    static void initialiseAudio(std::unique_ptr<AudioProcessorGraph> graph, std::unique_ptr<AudioDeviceManager> dm,
                                std::unique_ptr<AudioProcessorPlayer> pp, std::unique_ptr<XmlElement> ptr);
    static void close();

    //===================================================================
    // Setter / Getter info
    ValueTree& getTree() { return tree; }

protected:
    ValueTree tree;
    OwnedArray<ConnectionLine> connections;
    std::unique_ptr<Component> gui;

    //====================================================================================
    // Lasso stuff (todo: simplify)
    LineComponent dragLine;
    LassoComponent<GuiObject::Ptr> lasso;
    bool intersectMode = true;

    void findLassoItemsInArea (Array <GuiObject::Ptr>& results, const Rectangle<int>& area) override;
    ReferenceCountedArray<GuiObject> componentsToSelect;
    static ComponentSelection selected;
    SelectedItemSet<GuiObject::Ptr>& getLassoSelection() override;
    //====================================================================================
    // Hover identifier and management
    void setHoverComponent(GuiObject::Ptr c);
    GuiObject::Ptr hoverComponent = nullptr;

    Component* effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree);
    static ConnectionPort::Ptr portToConnectTo(MouseEvent& event, const ValueTree& effectTree);
    //====================================================================================
    void addEffect(const MouseEvent& event, const Effect& childEffect, bool addToMain = true);

    static void addAudioConnection(ConnectionLine& connectionLine);

    static std::unique_ptr<AudioDeviceManager> deviceManager;
    static std::unique_ptr<AudioProcessorPlayer> processorPlayer;
    static std::unique_ptr<AudioProcessorGraph> audioGraph;

    static UndoManager undoManager;
};



/**
 * Effect (ValueTree) is an encapsulator for individual or combinations of AudioProcessors
 *
 * This includes GUI, AudioProcessor, and the Effect's ValueTree itself.
 *
 * The valuetree data structure is itself owned by this object, and has a property reference to the owner object.
 */

class Effect : public EffectTreeBase
{
public:

    Effect(const MouseEvent &event, Array<const Effect*> effectVTSet);
    Effect(const MouseEvent &event, AudioProcessorGraph::NodeID nodeID);
    explicit Effect(const MouseEvent &event);

    ~Effect();

    using Ptr = ReferenceCountedObjectPtr<Effect>;
    void addConnection(ConnectionLine* connection);

    // ================================================================================
    // Effect tree data

    // Undoable actions
    void setPos(Point<int> newPos) {
        pos.x = newPos.x;
        pos.y = newPos.y;
    }

    struct Pos {
        CachedValue<int> x;
        CachedValue<int> y;
    } pos;

    void setParent(EffectTreeBase& parent) {
        parent.getTree().appendChild(tree, &undoManager);
    }

    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    void valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) override;

    // =================================================================================
    // Setters and getter functions

    struct NodeAndPort {
        AudioProcessorGraph::Node::Ptr node = nullptr;
        AudioPort* port = nullptr;
        bool isValid = false;
    };
    NodeAndPort getNode(ConnectionPort::Ptr& port);
    AudioProcessorGraph::NodeID getNodeID() const;

    // Data
    const String& getName() const { return name; }
    void setName(const String &name) { this->name = name; }
    ValueTree& getTree() {return tree;}
    const ValueTree& getTree() const {return tree;}
    GuiEffect* getGUIEffect() {return &guiEffect;}
    const GuiEffect* getGUIEffect() const {return &guiEffect;}

    AudioProcessor::Bus* getDefaultBus() { audioGraph->getBus(true, 0); }

    // Convenience functions
    Effect::Ptr getParent(){ return dynamic_cast<Effect*>(
                tree.getParent().getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
    Effect::Ptr getChild(int index){ return dynamic_cast<Effect*>(
                tree.getChild(index).getProperty(ID_EVT_OBJECT).getObject())->ptr(); }
    Effect::Ptr ptr(){ return this; }
    int getNumChildren(){ return tree.getNumChildren(); }
    bool isIndividual() const { return guiEffect.isIndividual(); }

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

private:
    // Used for an individual processor Effect. - does not contain anything else
    AudioProcessor* processor = nullptr;
    AudioProcessorGraph::Node::Ptr node = nullptr;

    ReferenceCountedArray<ConnectionLine> connections;

    String name;
    GuiEffect guiEffect;

    struct IDs {
        static const Identifier xPos;
        static const Identifier yPos;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Effect)

};

