/*
  ==============================================================================

    Effect.h
    Created: 31 Mar 2020 9:03:16pm
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>
#include "EffectGui.h"

#include "IOEffects.h"
#include "BaseEffects.h"

#pragma once

class Effect;

/**
 * Base class for Effects and EffectScene
 * Contains all functionality common to both:
 * Convenience functions for tree navigation,
 * Data for saving and loading state,
 */
class EffectTreeBase : public SelectHoverObject, public ValueTree::Listener, public LassoSource<GuiObject::Ptr>
{
public:
    explicit EffectTreeBase(ValueTree &vt) {
        tree = vt;
        tree.setProperty(IDs::effectTreeBase, this, nullptr);
        setWantsKeyboardFocus(true);

        dragLine.setAlwaysOnTop(true);
        LineComponent::setDragLine(&dragLine);
    }

    explicit EffectTreeBase(Identifier id) : tree(id) {
        tree.setProperty(IDs::effectTreeBase, this, nullptr);
        setWantsKeyboardFocus(true);

        dragLine.setAlwaysOnTop(true);
        LineComponent::setDragLine(&dragLine);
    }

    ~EffectTreeBase() override;

    void createConnection(std::unique_ptr<ConnectionLine> line);

    static void initialiseAudio(std::unique_ptr<AudioProcessorGraph> graph, std::unique_ptr<AudioDeviceManager> dm,
                                std::unique_ptr<AudioProcessorPlayer> pp, std::unique_ptr<XmlElement> ptr);
    static void close();
    virtual void resized() override = 0;

    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    bool keyPressed(const KeyPress &key) override;

    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    void valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                               int indexFromWhichChildWasRemoved) override;

    //===================================================================

    ValueTree& getTree() { return tree; }
    const ValueTree& getTree() const {return tree; }

    EffectTreeBase* getParent() { return dynamic_cast<EffectTreeBase*>(tree.getParent().getProperty(IDs::effectTreeBase).getObject()); }

    template<class T>
    T* getFromTree(ValueTree& vt);

protected:
    ValueTree tree;
    OwnedArray<ConnectionLine> connections;

    //====================================================================================
    // Menu stuff
    PopupMenu getEffectSelectMenu(const MouseEvent &event);

    //====================================================================================
    // Lasso stuff (todo: simplify)
    LassoComponent<GuiObject::Ptr> lasso;
    bool intersectMode = true;

    void findLassoItemsInArea (Array <GuiObject::Ptr>& results, const Rectangle<int>& area) override;

    SelectedItemSet<GuiObject::Ptr>& getLassoSelection() override;
    //====================================================================================
    // Hover identifier and management
    static LineComponent dragLine;

    Point<int> dragDetachFromParentComponent();

    EffectTreeBase* effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree);
    static ConnectionPort::Ptr portToConnectTo(MouseEvent& event, const ValueTree& effectTree);
    //====================================================================================

    static void addAudioConnection(ConnectionLine& connectionLine);

    static std::unique_ptr<AudioDeviceManager> deviceManager;
    static std::unique_ptr<AudioProcessorPlayer> processorPlayer;
    static std::unique_ptr<AudioProcessorGraph> audioGraph;

    static UndoManager undoManager;

    struct IDs {
    public:
        static const Identifier effectTreeBase;
        static const Identifier pos;
        static const Identifier processorID;
        static const Identifier initialised;
    };

};

struct Position : public VariantConverter<Point<int>>
{
    static Point<int> fromVar (const var &v);
    static var toVar (const Point<int> &t);
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
    explicit Effect(ValueTree& vt);

    void setupTitle();
    void setupMenu();

    ~Effect() override;

    void resized() override;
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    bool keyPressed(const KeyPress &key) override;

    using Ptr = ReferenceCountedObjectPtr<Effect>;
    void addConnection(ConnectionLine* connection);

    struct NodeAndPort {
        AudioProcessorGraph::Node::Ptr node = nullptr;
        AudioPort* port = nullptr;
        bool isValid = false;
    };
    NodeAndPort getNode(ConnectionPort::Ptr& port);

    void setParameters(const AudioProcessorParameterGroup* group);
    void addParameter(AudioProcessorParameter* param);
    void addPort(AudioProcessor::Bus* bus, bool isInput);

    ConnectionPort::Ptr checkPort(Point<int> pos);

    void setEditMode(bool isEditMode);
    bool isInEditMode() { return editMode; }
    // ================================================================================
    // Effect tree data

    // Undoable actions
    void setPos(Point<int> newPos);
    void setParent(EffectTreeBase& parent);

    CachedValue<Array<var>> pos;

    // ValueTree listener overrides
    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    void valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) override;

    // =================================================================================
    // Setters and getter functions

    void setProcessor(AudioProcessor* processor);

    void setName(const String &name) override;
    CustomMenuItems& getMenu() { return editMode ? editMenu : menu; }
    AudioProcessorGraph::NodeID getNodeID() const;
    AudioProcessor::Bus* getDefaultBus() { audioGraph->getBus(true, 0); }

    bool isIndividual() const { return processor != nullptr; }

private:
    // Used for an individual processor Effect. - does not contain anything else
    AudioProcessor* processor = nullptr;
    AudioProcessorGraph::Node::Ptr node = nullptr;

    ReferenceCountedArray<ConnectionLine> connections;

    String name;

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

    const AudioProcessorParameterGroup* parameters;

    int portIncrement = 50;
    int inputPortStartPos = 100;
    int inputPortPos = inputPortStartPos;
    int outputPortStartPos = 100;
    int outputPortPos = outputPortStartPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Effect)

};

