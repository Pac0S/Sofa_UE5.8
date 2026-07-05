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

UENUM(BlueprintType)
enum class ESofaToolControlMode : uint8
{
    None        UMETA(DisplayName = "None"),
    PoseTarget  UMETA(DisplayName = "PoseTarget")
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

USTRUCT(BlueprintType)
struct SOFABRIDGE_API FSofaToolInputState
{
    GENERATED_BODY()
    FName ToolId = NAME_None;
    FTransform TargetPose = FTransform::Identity;
    bool bEnabled = true;
    double Timestamp = 0.0;
};

USTRUCT(BlueprintType)
struct SOFABRIDGE_API FSofaFrameInput
{
    GENERATED_BODY()
    TArray<FSofaToolInputState> Tools;
};