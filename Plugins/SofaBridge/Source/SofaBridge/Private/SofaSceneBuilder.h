#pragma once

#include "CoreMinimal.h"
#include "SofaSceneLoader.h"
#include "SofaSceneTypes.h"

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