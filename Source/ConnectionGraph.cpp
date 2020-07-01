/*
  ==============================================================================

    ConnectionGraph.cpp
    Created: 1 Jul 2020 12:18:22pm
    Author:  maxime

  ==============================================================================
*/

#include "ConnectionGraph.h"

ConnectionGraph::ConnectionGraph(AudioProcessorGraph& processorGraph)
    : ComponentListener()
    , audioGraph(processorGraph)
{

}
