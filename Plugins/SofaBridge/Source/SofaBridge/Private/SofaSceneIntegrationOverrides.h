#pragma once

#include "CoreMinimal.h"
#include "SofaRuntimeEnums.h"

#include "SofaSceneIntegrationOverrides.generated.h"

USTRUCT()
struct FSofaObjectIntegrationOverride
{
    GENERATED_BODY()

    UPROPERTY()
    FString ObjectId;

    UPROPERTY()
    FString VisualMaterialPath;

    UPROPERTY()
    float SofaScale = 10.0f;

    UPROPERTY()
    FVector UnrealTranslation = FVector::ZeroVector;

    UPROPERTY()
    FRotator UnrealRotation = FRotator::ZeroRotator;

    UPROPERTY()
    bool bPreferVisualSurface = true;

    UPROPERTY()
    ESofaRuntimeObjectRole Role = ESofaRuntimeObjectRole::Unknown;
};

USTRUCT()
struct FSofaSceneIntegrationOverrides
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSofaObjectIntegrationOverride> Objects;
};