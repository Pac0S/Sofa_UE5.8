#pragma once

#include "CoreMinimal.h"

struct FSofaVisualObjectConfig
{
    FString VisualMeshPath;
    FString TexturePath;
    FString MaterialPath;

    bool bUseVisualMesh = true;
    bool bUseTexture = true;
    bool bHandleSeams = true;
    FVector2D TextureScale = FVector2D(1.0, 1.0);
    FVector2D TextureOffset = FVector2D(0.0, 0.0);
};

struct FSofaDeformableObjectConfig
{
    FString Id = TEXT("SoftBody_01");
    FString VolumeMeshPath;

    FSofaVisualObjectConfig VisualConfig;

    double Scale = 1.0;
    double TotalMass = 1.0;
    double YoungModulus = 3000.0;
    double PoissonRatio = 0.3;

    FVector Translation = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    bool bFixedBase = false;
};

struct FSofaSceneConfig
{
    FString SceneId = TEXT("PrototypeScene");
    double TimeStep = 0.01;
    FVector Gravity = FVector(0.0, -9.81, 0.0);

    TArray<FSofaDeformableObjectConfig> Objects;
};