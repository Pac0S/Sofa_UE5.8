#pragma once

#include "CoreMinimal.h"
#include "SofaRuntimeEnums.h"

#include "SofaSceneIntegrationOverrides.generated.h"

UENUM()
enum class ESofaNodeRefRole : uint8
{
    None,
    Simulation,
    Visual,
    Surface,
    Collision,
    Control
};

USTRUCT()
struct FSofaNodeRef
{
    GENERATED_BODY()

    UPROPERTY()
    ESofaNodeRefRole Role = ESofaNodeRefRole::None;

    UPROPERTY()
    FName Name;
};

USTRUCT()
struct FSofaObjectIntegrationOverride
{
    GENERATED_BODY()

    UPROPERTY()
    FString ObjectId;

    UPROPERTY()
    FString VisualMaterialPath;

    UPROPERTY()
    FString StaticMeshPath;

    UPROPERTY()
    TArray<FSofaNodeRef> NodeRefs;

    UPROPERTY()
    float SofaScale = 10.0f;

    UPROPERTY()
    bool bPreferVisualSurface = true;

    UPROPERTY()
    ESofaRuntimeObjectRole Role = ESofaRuntimeObjectRole::Unknown;
};

USTRUCT(BlueprintType)
struct FSofaToolIntegrationOverride
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ToolId;

    UPROPERTY()
    TArray<FSofaNodeRef> NodeRefs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SofaScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVisible = true;
};

USTRUCT()
struct FSofaSceneIntegrationOverrides
{
    GENERATED_BODY()

    UPROPERTY()


    TArray<FSofaObjectIntegrationOverride> Objects;

    UPROPERTY()
    TArray<FSofaToolIntegrationOverride> Tools;
};