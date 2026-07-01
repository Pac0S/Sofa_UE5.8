#pragma once

#include "CoreMinimal.h"
#include "SofaSceneConfig.h"
#include "SofaSimulationService.h"
#include "SofaSceneLoader.h"

struct FSofaRuntimeScene;

class FSofaSceneBuilder
{
public:
    struct FBuildResult
    {
        bool bSuccess = false;
        FString ErrorMessage;
    };

public:
    static FBuildResult BuildPrototypeScene(
        FSofaRuntimeScene& SofaContext,
        const FSofaPrototypeSceneRequest& Request);
};