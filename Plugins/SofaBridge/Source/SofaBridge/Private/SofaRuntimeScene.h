#pragma once

#include "SofaIncludes.h"
#include "SofaRuntimeTypes.h"

struct FSofaResolvedBinding
{
    FSofaMechanicalBindingDescriptor Descriptor;
    uint64 Generation = 0;
    bool bResolved = false;
    bool IsValidForGeneration(uint64 InGeneration) const
    {
        return bResolved && Generation == InGeneration;
    }
};

struct FSofaIndexedNode
{
    FString Path;
    FString Name;
    uint64 Generation = 0;
};

struct FSofaIndexedObject
{
    FString NodePath;
    FString ObjectKey;
    FString ObjectName;
    FString ClassName;
    uint64 Generation = 0;
};

struct FSofaRuntimeScene
{
    sofa::simulation::Simulation::SPtr SimulationPtr;
    sofa::simulation::NodeSPtr RootNode;
    FString LoadedScenePath;
    FString SceneName;

    TArray<FSofaRuntimeObjectDescriptor> RuntimeObjects;
    TArray<FSofaRuntimeToolDescriptor> RuntimeTools;
    TArray<FSofaResolvedBinding> Bindings;

    TMap<FString, FSofaIndexedNode> NodeIndexByPath;
    TMap<FString, FSofaIndexedObject> ObjectIndexByKey;
    TMultiMap<FString, FString> ObjectKeysByName;

    uint64 SceneGeneration = 0;
};