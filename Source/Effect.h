/*
  ==============================================================================

    Effect.h
    Created: 31 Mar 2020 9:03:16pm
    Author:  maxime

  ==============================================================================
*/

#include <JuceHeader.h>
#include "EffectLoader.h" //todo remove
#include "EffectGui.h"
#include "ConnectionLine.h"
#include "Parameters.h"
#include "Ports.h"
#include "MenuItem.h"
#include "ConnectionGraph.h" //todo remove
#include "WeakReferenceContainer.h"
#include "UI/SceneComponent.h"


#pragma once

class Effect;


/**
 * Base class between Effects and EffectScene
 * Contains all functionality common to both:
 * Convenience functions for tree navigation,
 * Data for saving and loading state,
 */
class EffectBase : public SceneComponent
{
public:
    EffectBase();

    void resized() override = 0;

    void mouseDown(const MouseEvent &event) override;

    void handleCommandMessage(int commandId) override;

    //===================================================================

    static bool connectParameters(const ConnectionLine& connectionLine);
    static void disconnectParameters(const ConnectionLine& connectionLine);

    static void disconnectAudio(const ConnectionLine& connectionLine);
    static bool connectAudio(const ConnectionLine& connectionLine);

    Array<ConnectionLine*> getConnectionsToThis(bool isInputConnection, ConnectionLine::Type connectionType);
    Array<ConnectionLine*> getConnectionsToThis();
    Array<ConnectionLine*> getConnectionsInside();

    //===================================================================

    static UndoManager& getUndoManager() { return undoManager; }
    static AudioDeviceManager* getDeviceManager() {return deviceManager;}

    ConnectionPort_old* getPortFromID(String portID);

    enum AppState {
        loading = 0,
        neutral = 1,
        stopping = 2
    }
    static appState;

protected:
    static EffectBase* effectScene;

    //====================================================================================
    // Connections management
    ConnectionLine* dragLine = nullptr;

    //====================================================================================

    static AudioDeviceManager* deviceManager;
    static UndoManager undoManager;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectBase)
};

/**
 * Effect (ValueTree) is an encapsulator for individual or combinations of AudioProcessors
 *
 * This includes GUI, AudioProcessor, and the Effect's ValueTree itself.
 *
 * The valuetree data structure is itself owned by this object, and has a property reference to the owner object.
 */

class Effect : public EffectBase, public MenuItem
{
public:
    Effect(String name, int processorID, bool editMode,
            int x, int y, int w, int h); //todo ports and parameters

    ReferenceCountedObjectPtr<Effect> masterReference;

    void setupTitle();
    void setupMenu();

    ~Effect() override;

    void resized() override;
    void paint(Graphics& g) override;

    void setName(const String &newName) override;

    void expandToFitChildren();
    void childrenChanged() override;

    Point<int> getPosWithinParent();

    void mouseDrag(const MouseEvent &event) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

    void mouseDoubleClick(const MouseEvent &event) override;

    bool canDragInto(const SceneComponent &other, bool isRightClickDrag) const override;
    bool canDragHover(const SceneComponent &other, bool isRightClickDrag) const override;

    struct NodeAndPort {
        AudioProcessorGraph::Node::Ptr node = nullptr;
        AudioPort* port = nullptr;
        bool isValid = false;
    };

    Array<Effect*> getFullConnectionEffects(ConnectionPort_old* port, bool includeParent = false);
    Array<Effect*> getFullConnectionEffectsInside();
    Array<ConnectionLine*> getConnectionsUntilEnd(ConnectionPort_old* port);
    ConnectionPort_old* getNextPort(ConnectionPort_old* port, bool stopAtProcessor = true);
    ConnectionPort_old* getEndPort(ConnectionPort_old* port);

    void setNode(AudioProcessorGraph::Node::Ptr node);

    //ValueTree createParameter(AudioProcessorParameter* param);
    //Parameter::Ptr loadParameter(ValueTree parameterData);

    Array<AudioProcessorParameter*> getParameters(bool recursive);
    Array<Parameter*> getParameterChildren();

    Parameter* getParameterForPort(ParameterPort* port);

    void addPort(AudioPort* port);
    AudioPort::Ptr addPort(AudioProcessor::Bus* bus, bool isInput);
    Array<ConnectionPort_old*> getPorts(int isInput = -1);
    void removePort(ConnectionPort_old* port);

    //ConnectionPort_old* checkPort(Point<int> pos);
    bool hasPort(const ConnectionPort_old* port);
    bool hasConnection(const ConnectionLine* line);

    //int getPortID(const ConnectionPort_old* port);
    //ConnectionPort_old* getPortFromID(const int id, bool internal = false);

    void setEditMode(bool isEditMode);
    bool isInEditMode() const { return editMode; }

    void repositionInternals();
    void updateConstrainerSize();

    // =================================================================================
    // Setters and getter functions
    void setProcessor(AudioProcessor* processor);
    AudioProcessor* getProcessor() const {return processor;}
    bool hasProcessor(AudioProcessor* processor);

    //CustomMenuItems& getMenu() { return editMode ? editMenu : menu; }
    AudioProcessorGraph::NodeID getNodeID() const;

    int getNumInputs() const { return inputPortFlexBox.items.size(); }
    int getNumOutputs() const { return outputPortFlexBox.items.size(); }

    bool isIndividual() const { return processor != nullptr; }

    struct IDs {
        static const Identifier EFFECT_ID;
        static const Identifier editMode;
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

    enum menu {
        editMenu = 0,
        menu = 1
    };

private:
    bool editMode = false;
    /*ReferenceCountedArray<AudioPort> inputPorts;
    ReferenceCountedArray<AudioPort> outputPorts;*/
    Label title;
    Image image;

    // Layout stuff
    FlexBox parameterFlexBox;
    FlexBox paramPortsFlexBox;
    FlexBox parameterFlexBoxEditMode;
    
    FlexBox inputPortFlexBox;
    FlexBox outputPortFlexBox;
    FlexBox internalInputPortFlexBox;
    FlexBox internalOutputPortFlexBox;

    Resizer resizer;

    Point<int> rightClickDragPos;
    bool rightClickDragActivated = false;

    ReferenceCountedArray<Parameter> parameterArray; //todo organise this into ParameterManager?

    //============================================================
    // Audio

    const AudioProcessorParameterGroup* parameters = nullptr;
    //AudioProcessorGraph graph; //todo graph contains processor, effects etc

    AudioProcessor* processor = nullptr;
    AudioProcessorGraph::Node::Ptr node = nullptr;

    //============================================================



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Effect)
};
