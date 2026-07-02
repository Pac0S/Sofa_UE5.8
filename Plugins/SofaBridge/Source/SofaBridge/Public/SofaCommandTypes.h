#pragma once

#include "CoreMinimal.h"
#include "SofaCommandTypes.generated.h"

UENUM(BlueprintType)
enum class ESofaCommandType : uint8
{
    None                        UMETA(DisplayName = "None"),
    LoadPrototypeScene          UMETA(DisplayName = "LoadPrototypeScene"),
    ResetScene                  UMETA(DisplayName = "ResetScene"),
    Pause                       UMETA(DisplayName = "Pause"),
    Resume                      UMETA(DisplayName = "Resume"),
    Stop                        UMETA(DisplayName = "Stop"),
    SetGravity                  UMETA(DisplayName = "SetGravity"),
    SetInteractorTargetPose     UMETA(DisplayName = "SetInteractorTargetPose"),
    ClearInteractorTargetPose   UMETA(DisplayName = "ClearInteractorTargetPose")
};

USTRUCT(BlueprintType)
struct SOFABRIDGE_API FSofaCommand
{
    GENERATED_BODY()
    ESofaCommandType Type = ESofaCommandType::None;
    FName TargetId = NAME_None;
    FTransform Transform = FTransform::Identity;
    FVector VectorValue = FVector::ZeroVector;
    FString StringValue;
    double Timestamp = 0.0;
};