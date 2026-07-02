#pragma once

#include "CoreMinimal.h"
#include "SofaRuntimeEnums.h"
#include "SofaSceneTypes.h"
#include "SofaRuntimeTypes.generated.h"

USTRUCT(BlueprintType)
struct SOFABRIDGE_API FSofaRuntimeObjectDescriptor
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString ObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    ESofaRuntimeObjectRole Role = ESofaRuntimeObjectRole::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString SimulationNodeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString MechanicalObjectName = TEXT("mstate");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString TopologyContainerName = TEXT("topo");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString SurfaceNodeName = TEXT("Surface");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString SurfaceTopologyName = TEXT("surfaceTopo");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString VisualNodeName = TEXT("Visual");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString VisualMechanicalObjectName = TEXT("visualDofs");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString VisualTopologyName = TEXT("visualTopo");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FString VisualMaterialPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FTransform UnrealObjectTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    float SofaScale = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    bool bVisible = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    bool bExtractSurface = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    bool bPreferVisualSurface = true;
};

USTRUCT(BlueprintType)
struct SOFABRIDGE_API FSofaDebugPoint
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
struct SOFABRIDGE_API FSofaDebugTriangle
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
struct SOFABRIDGE_API FSofaSurfaceMeshState
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
struct SOFABRIDGE_API FSofaObjectState
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
struct SOFABRIDGE_API FSofaFrameSnapshot
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