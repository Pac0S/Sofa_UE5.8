#pragma once

#include "SofaIncludes.h"
#include "SofaRuntimeTypes.h"

struct FSofaRuntimeScene
{
    sofa::simulation::Simulation::SPtr SimulationPtr;
    sofa::simulation::NodeSPtr RootNode;
    FString LoadedScenePath;
    FString SceneName;
    TArray<FSofaRuntimeObjectDescriptor> RuntimeObjects;
};
