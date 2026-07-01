#pragma once

#include "SofaIncludes.h"

struct FSofaRuntimeScene
{
    sofa::simulation::Simulation::SPtr SimulationPtr;
    sofa::simulation::NodeSPtr RootNode;
};