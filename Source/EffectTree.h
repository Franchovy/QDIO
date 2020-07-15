/*
  ==============================================================================

    EffectTree.h
    Created: 8 May 2020 9:42:07am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include "Effect.h"
#include <BinaryData.h>
#include "IDs"
#include "EffectPositioner.h"

class EffectTree : public ValueTree::Listener, public ComponentListener
{
public:
    EffectTree(EffectBase* effectScene);
    ~EffectTree();

    struct IDs {
        static Identifier component;
    };

    // Create new - from ValueTree data
    bool createEffect(ValueTree tree);

    // Create new methods - constructs ValueTree Data needed
    ValueTree newEffect(String name, Point<int> pos, int processorID = -1);

    StringArray getProcessorNames();
    String getProcessorName(int processorID);
    std::unique_ptr<AudioProcessor> createProcessor(int processorID);
    void setupProcessors();

    void remove(SelectHoverObject* c);

    ValueTree storeGroup(Array<SelectHoverObject*> items);

    ValueTree storeEffect(const ValueTree& storeData);
    EffectBase* loadEffect(ValueTree& parentTree, const ValueTree& loadData);
    bool loadEffect(const ValueTree& loadData, bool setToMousePosition = true);

    bool loadPort(ValueTree port);
    bool loadParameter(Effect* effect, ValueTree parameterData);
    bool loadConnection(ValueTree connectionData);

    String getCurrentTemplateName();

    bool loadTemplate(String name = "");
    void storeTemplate(String saveName = "");

    void clear();
    bool isNotEmpty();

    // Add and remove ValueTree functions (undoable)
    void valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                               int indexFromWhichChildWasRemoved) override;
    void valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) override;
    void valueTreeParentChanged(ValueTree &treeWhoseParentHasChanged) override;

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;
    void componentNameChanged(Component &component) override;
    void componentChildrenChanged(Component &component) override;
    void componentEnablementChanged(Component &component) override;
    void componentParentHierarchyChanged(Component &component) override;
    void componentBeingDeleted(Component &component) override;

    // todo methods to move to EffectManager class
    ValueTree getUsefulSelection();


    // Access methods
    ValueTree getTree(GuiObject* component);
    ValueTree findTree(ValueTree treeToSearch, GuiObject* component);

    template<class T>
    static T* getFromTree(const ValueTree& vt);
    template <class T>
    static T* getPropertyFromTree(const ValueTree &vt, Identifier property);


    void removeListenersRecursively(Component* component);

private:
    EffectBase* effectScene;

    StringArray processorNames;
    Array<std::function<void()>> makeProcessorArray;
    std::unique_ptr<AudioProcessor> newProcessor = nullptr;

    String currentTemplateName;

    ValueTree effectTree;
    UndoManager* undoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectTree)
};
