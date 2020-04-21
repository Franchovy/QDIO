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


struct ConnectionVar : public VariantConverter<ReferenceCountedArray<ConnectionLine>>
{
    static ReferenceCountedArray<ConnectionLine> fromVarArray (const var &v);
    static var toVarArray (const ReferenceCountedArray<ConnectionLine> &t);
    static ConnectionLine::Ptr fromVar (const var &v);
    static var* toVar (const ConnectionLine::Ptr &t);

};

/**
 * Base class for Effects and EffectScene
 * Contains all functionality common to both:
 * Convenience functions for tree navigation,
 * Data for saving and loading state,
 */
class EffectTreeBase : public SelectHoverObject, public ValueTree::Listener, public LassoSource<GuiObject::Ptr>
{
public:
    explicit EffectTreeBase(const ValueTree &vt);

    explicit EffectTreeBase(Identifier id);

    ~EffectTreeBase() override;

    void createConnection(ConnectionLine::Ptr line);

    static void close();

    static ValueTree storeEffect(ValueTree& tree);
    static void loadEffect(ValueTree& parentTree, ValueTree& loadData);

    void resized() override = 0;
    bool keyPressed(const KeyPress &key) override;

    //void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    void valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                               int indexFromWhichChildWasRemoved) override;

    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;

    //===================================================================

    ValueTree& getTree() { return tree; }
    const ValueTree& getTree() const {return tree; }

    EffectTreeBase* getParent() { return dynamic_cast<EffectTreeBase*>(tree.getParent().getProperty(IDs::effectTreeBase).getObject()); }

    template<class T>
    static T* getFromTree(const ValueTree& vt);

    template <class T>
    static T* getPropertyFromTree(const ValueTree &vt, Identifier property);


    static ReferenceCountedArray<Effect> effectsToDelete;

    struct IDs {
        static const Identifier effectTreeBase;
    };

    enum AppState {
        loading = 0,
        neutral = 1
    } static appState;

protected:
    ValueTree tree;

    //====================================================================================
    // Menu stuff
    static void callMenu(PopupMenu& menu);

    PopupMenu createEffectMenu;
    PopupMenu getEffectSelectMenu();
    void newEffect(String name, int processorID);

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

    static EffectTreeBase* effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree);
    static ConnectionPort::Ptr portToConnectTo(const MouseEvent& event, const ValueTree& effectTree);
    //====================================================================================

    static void disconnectAudio(const ConnectionLine& connectionLine);
    static bool connectAudio(const ConnectionLine& connectionLine);
    static Array<AudioProcessorGraph::Connection> getAudioConnection(const ConnectionLine& connectionLine);

    static void createGroupEffect();

    static AudioDeviceManager* deviceManager;
    static AudioProcessorPlayer* processorPlayer;
    static AudioProcessorGraph* audioGraph;

    static UndoManager undoManager;


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectTreeBase)
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
    explicit Effect(const ValueTree& vt);

    void setupTitle();
    void setupMenu();

    ~Effect() override;

    void resized() override;
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    using Ptr = ReferenceCountedObjectPtr<Effect>;

    void hoverOver(EffectTreeBase* newParent);
    void reassignNewParent(EffectTreeBase* newParent);

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
    bool hasPort(const ConnectionPort* port);

    int getPortID(const ConnectionPort* port);
    AudioPort* getPortFromID(const int id);

    void setEditMode(bool isEditMode);
    bool isInEditMode() { return editMode; }
    // ================================================================================
    // Effect tree data

    // Undoable actions
    void setPos(Point<int> newPos);
    void resize(int w, int h);
    void setParent(EffectTreeBase& parent);

    static void updateEffectProcessor(AudioProcessor* processorToUpdate, ValueTree treeToSearch);

    //CachedValue<Array<var>> pos;

    // ValueTree listener overrides
    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    void valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) override;


    // =================================================================================
    // Setters and getter functions
    bool hasProcessor(AudioProcessor* processor);

    //CustomMenuItems& getMenu() { return editMode ? editMenu : menu; }
    AudioProcessorGraph::NodeID getNodeID() const;
    AudioProcessor::Bus* getDefaultBus() { audioGraph->getBus(true, 0); }

    bool isIndividual() const { return processor != nullptr; }

    struct IDs {
        static const Identifier EFFECT_ID;
        //static const Identifier pos;
        static const Identifier x;
        static const Identifier y;
        static const Identifier w;
        static const Identifier h;
        static const Identifier processorID;
        static const Identifier initialised;
        static const Identifier name;
        static const Identifier connections;

    };

private:
    // Used for an individual processor Effect. - does not contain anything else
    void setProcessor(AudioProcessor* processor);

    bool editMode = false;
    ReferenceCountedArray<AudioPort> inputPorts;
    ReferenceCountedArray<AudioPort> outputPorts;
    Label title;
    Image image;

    Resizer resizer;
    ComponentDragger dragger;
    ComponentBoundsConstrainer constrainer;

    PopupMenu menu;
    PopupMenu editMenu;

    const AudioProcessorParameterGroup* parameters;

    AudioProcessor* processor = nullptr;
    AudioProcessorGraph::Node::Ptr node = nullptr;

    int portIncrement = 50;
    int inputPortStartPos = 100;
    int inputPortPos = inputPortStartPos;
    int outputPortStartPos = 100;
    int outputPortPos = outputPortStartPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Effect)

};

