/*
  ==============================================================================

    Effect.h
    Created: 31 Mar 2020 9:03:16pm
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>
#include "EffectLoader.h"
#include "EffectGui.h"
#include "ConnectionLine.h"
#include "Parameters.h"
#include "Ports.h"
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
class EffectTreeBase : public SelectHoverObject, public ValueTree::Listener, public LassoSource<SelectHoverObject::Ptr>
{
public:
    EffectTreeBase();

    static void close();

    void resized() override = 0;

    //===================================================================

    static void disconnectAudio(const ConnectionLine& connectionLine);
    static bool connectAudio(const ConnectionLine& connectionLine);
    static Array<AudioProcessorGraph::Connection> getAudioConnection(const ConnectionLine& connectionLine);

    //===================================================================

    static UndoManager& getUndoManager() { return undoManager; }
    static AudioDeviceManager* getDeviceManager() {return deviceManager;}
    static AudioProcessorGraph* getAudioGraph() {return audioGraph;}

/*
    ValueTree& getTree() { return tree; }
    const ValueTree& getTree() const {return tree; }
*/
    EffectTreeBase* getParent() { return dynamic_cast<EffectTreeBase*>(getParentComponent()); }



    struct IDs {
        static const Identifier effectTreeBase;
    };

    enum AppState {
        loading = 0,
        neutral = 1
    } static appState;

protected:
    //ValueTree tree;

    //====================================================================================
    // Menu stuff
    void callMenu(PopupMenu& menu);

    //====================================================================================
    // Lasso stuff (todo: simplify)
    LassoComponent<GuiObject::Ptr> lasso;
    bool intersectMode = true;

    void findLassoItemsInArea (Array <SelectHoverObject::Ptr>& results, const Rectangle<int>& area) override;
    SelectedItemSet<SelectHoverObject::Ptr>& getLassoSelection() override;

    //====================================================================================
    // Hover identifier and management
    static LineComponent dragLine;

    static EffectTreeBase* effectToMoveTo(const MouseEvent& event, const ValueTree& effectTree);
    static ConnectionPort* portToConnectTo(const MouseEvent& event, const ValueTree& effectTree);
    //====================================================================================

    void createGroupEffect();

    //====================================================================================

    static AudioDeviceManager* deviceManager;
    static AudioProcessorPlayer* processorPlayer;
    static AudioProcessorGraph* audioGraph;

    static UndoManager undoManager;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectTreeBase)
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
    Effect();

    void setupTitle();
    void setupMenu();

    ~Effect() override;

    void resized() override;
    void paint(Graphics& g) override;

    using Ptr = ReferenceCountedObjectPtr<Effect>;


    struct NodeAndPort {
        AudioProcessorGraph::Node::Ptr node = nullptr;
        AudioPort* port = nullptr;
        bool isValid = false;
    };

    NodeAndPort getNode(ConnectionPort::Ptr& port);
    void setNode(AudioProcessorGraph::Node::Ptr node);

    ValueTree createParameter(AudioProcessorParameter* param);
    Parameter::Ptr loadParameter(ValueTree parameterData);

    Array<AudioProcessorParameter*> getParameters(bool recursive);
    Array<Parameter*> getParameterChildren();

    Parameter* getParameterForPort(ParameterPort* port);

    AudioPort::Ptr addPort(AudioProcessor::Bus* bus, bool isInput);
    Array<ConnectionPort*> getPorts(int isInput = -1);

    ConnectionPort* checkPort(Point<int> pos);
    bool hasPort(const ConnectionPort* port);
    bool hasConnection(const ConnectionLine* line);

    int getPortID(const ConnectionPort* port);
    ConnectionPort* getPortFromID(const int id, bool internal = false);

    void setEditMode(bool isEditMode);
    bool isInEditMode() { return editMode; }

    // =================================================================================
    // Setters and getter functions
    void setProcessor(AudioProcessor* processor);
    AudioProcessor* getProcessor() const {return processor;}
    bool hasProcessor(AudioProcessor* processor);

    //CustomMenuItems& getMenu() { return editMode ? editMenu : menu; }
    AudioProcessorGraph::NodeID getNodeID() const;
    static AudioProcessor::Bus* getDefaultBus() { audioGraph->getBus(true, 0); }

    int getNumInputs() const { return inputPorts.size(); }
    int getNumOutputs() const { return outputPorts.size(); }

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

    ComponentDragger dragger;
    ComponentBoundsConstrainer constrainer;

private:
    bool editMode = false;
    ReferenceCountedArray<AudioPort> inputPorts;
    ReferenceCountedArray<AudioPort> outputPorts;
    Label title;
    Image image;

    Resizer resizer;

    PopupMenu menu;
    PopupMenu editMenu;

    const AudioProcessorParameterGroup* parameters = nullptr;
    ReferenceCountedArray<Parameter> parameterArray;

    AudioProcessor* processor = nullptr;
    AudioProcessorGraph::Node::Ptr node = nullptr;

    int portIncrement = 50;
    int inputPortStartPos = 100;
    int inputPortPos = inputPortStartPos;
    int outputPortStartPos = 100;
    int outputPortPos = outputPortStartPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Effect)

};
