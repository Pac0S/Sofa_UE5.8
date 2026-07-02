#pragma once

#include "CoreMinimal.h"
#include "SofaSceneTypes.generated.h"

UENUM(BlueprintType)
enum class ESofaSimState : uint8
{
    Stopped     UMETA(DisplayName = "Stopped"),
    Starting    UMETA(DisplayName = "Starting"),
    Running     UMETA(DisplayName = "Running"),
    Paused      UMETA(DisplayName = "Paused"),
    Stopping    UMETA(DisplayName = "Stopping"),
    Error       UMETA(DisplayName = "Error")
};

UENUM()
enum class ESofaSurfaceSource : uint8
{
    None            UMETA(DisplayName = "None"),
    VisualSurface   UMETA(DisplayName = "VisualSurface"),
    DerivedSurface  UMETA(DisplayName = "DerivedSurface")
};

USTRUCT(BlueprintType)
struct SOFABRIDGE_API FSofaPrototypeSceneRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    bool bUseSceneFilePath = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString SceneFilePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString SceneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString ExternalScenesDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString RelativeScenesDirectory = TEXT("SofaScenes");
};