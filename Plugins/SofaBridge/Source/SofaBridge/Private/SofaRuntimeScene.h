#pragma once

#include "SofaIncludes.h"

struct FSofaRuntimeObjectDescriptor
{
    FString ObjectId;
    FString SimulationNodeName;

    FString MechanicalObjectName = TEXT("mstate");
    FString TopologyContainerName = TEXT("topo");

    FString SurfaceNodeName = TEXT("Surface");
    FString SurfaceTopologyName = TEXT("surfaceTopo");

    FString VisualNodeName = TEXT("Visual");
    FString VisualMechanicalObjectName = TEXT("visualDofs");
    FString VisualTopologyName = TEXT("visualTopo");
    FString VisualMaterialPath;

    bool bPreferVisualSurface = true;

    FTransform UnrealObjectTransform = FTransform::Identity;
    float SofaScale = 10.0f;
};

struct FSofaRuntimeScene
{
    sofa::simulation::Simulation::SPtr SimulationPtr;
    sofa::simulation::NodeSPtr RootNode;
    FString LoadedScenePath;
    FString SceneName;
    TArray<FSofaRuntimeObjectDescriptor> RuntimeObjects;
};
