/*
  ==============================================================================

    EffectTree.h
    Created: 8 May 2020 9:42:07am
    Author:  maxime

  ==============================================================================
*/

#pragma once

#include "DSPEffects.h"
#include "Effect.h"
#include "IDs"

class EffectTree : public ValueTree::Listener, public ComponentListener
{
public:
    EffectTree(EffectTreeBase* effectScene);
    ~EffectTree();

    // Create new - from ValueTree data
    Effect* loadEffect(ValueTree tree);


    // Create new methods - constructs ValueTree Data needed
    ValueTree newConnection(ConnectionPort::Ptr inPort, ConnectionPort::Ptr outPort);
    ValueTree newEffect(String name, Point<int> pos, int processorID = -1);
    ValueTree newParameter();

    void remove(SelectHoverObject* c);

    ValueTree storeEffect(const ValueTree& tree);
    void loadEffect(ValueTree& parentTree, const ValueTree& loadData);

    void loadUserState();
    void storeAll();

    // Add and remove ValueTree functions (undoable)
    void valueTreeChildAdded(ValueTree &parentTree, ValueTree &childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree &parentTree, ValueTree &childWhichHasBeenRemoved,
                               int indexFromWhichChildWasRemoved) override;

    void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;
    void componentNameChanged(Component &component) override;
    void componentChildrenChanged(Component &component) override;



    // Convenience methods
    ValueTree getTree(EffectTreeBase* effect);
    template<class T>
    static T* getFromTree(const ValueTree& vt);
    template <class T>
    static T* getPropertyFromTree(const ValueTree &vt, Identifier property);


private:
    ValueTree effectTree;
    UndoManager* undoManager;

    ReferenceCountedArray<Effect> effectsToDelete;
};
