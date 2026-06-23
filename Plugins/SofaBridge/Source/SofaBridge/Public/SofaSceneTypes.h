#pragma once

#include "CoreMinimal.h"

enum class ESofaSimState : uint8
{
    Stopped,
    Starting,
    Running,
    Paused,
    Stopping,
    Error
};

enum class ESofaCommandType : uint8
{
    None,
    LoadPrototypeScene,
    ResetScene,
    Pause,
    Resume,
    Stop,
    SetInteractorTargetPose,
    SetGravity
};

struct FSofaCommand
{
    ESofaCommandType Type = ESofaCommandType::None;
    FName TargetId = NAME_None;
    FTransform Transform = FTransform::Identity;
    FVector VectorValue = FVector::Zero();
    FString StringValue;
    double Timestamp = 0.0;
};

struct FSofaObjectState
{
    FName ObjectId = NAME_None;
    FTransform WorldTransform = FTransform::Identity;
    FVector LinearVelocity = FVector::ZeroVector;
    bool bInContact = false;
};

struct FSofaFrameSnapshot
{
    uint64 FrameId = 0;
    double SimTime = 0.0;
    ESofaSimState State = ESofaSimState::Stopped;
    TArray<FSofaObjectState> Objects;
};