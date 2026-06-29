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

UENUM()
enum class ESofaSurfaceSource : uint8
{
    None            UMETA(DisplayName = "None"),
    VisualSurface   UMETA(DisplayName = "VisualSurface"),
    DerivedSurface  UMETA(DisplayName = "DerivedSurface")
};


USTRUCT(BlueprintType)
struct FSofaCommand
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
struct FSofaDebugPoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FColor Color = FColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    float Size = 6.0f;
};

USTRUCT(BlueprintType)
struct FSofaDebugTriangle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    int32 A = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    int32 B = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    int32 C = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FSofaSurfaceMeshState
{
    GENERATED_BODY()

    UPROPERTY()
    ESofaSurfaceSource Source = ESofaSurfaceSource::None;

    UPROPERTY()
    TArray<FVector> Vertices;

    UPROPERTY()
    TArray<int32> Triangles;

    UPROPERTY()
    TArray<FVector> Normals;

    UPROPERTY()
    TArray<FVector2D> UV0;
};

USTRUCT(BlueprintType)
struct FSofaObjectState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FName ObjectId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FTransform WorldTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FVector LinearVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    bool bInContact = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    TArray<FSofaDebugPoint> DebugPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    TArray<FSofaDebugTriangle> SurfaceTriangles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FSofaSurfaceMeshState SurfaceMesh;
};

USTRUCT(BlueprintType)
struct FSofaFrameSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    int64 FrameId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    double SimTime = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    ESofaSimState State = ESofaSimState::Stopped;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    TArray<FSofaObjectState> Objects;
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