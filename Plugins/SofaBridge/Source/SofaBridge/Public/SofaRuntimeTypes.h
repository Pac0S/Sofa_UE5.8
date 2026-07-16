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
    ESofaRuntimeObjectRole Role = ESofaRuntimeObjectRole::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString ObjectNodeName;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString StaticMeshPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString CollisionNodeName = TEXT("Collision");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FString CollisionObjectName = TEXT("collisionDofs");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    FTransform InitialLocalTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    float SofaScale = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FVector SofaScale3D = FVector(1.0, 1.0, 1.0);

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
    FVector LinearVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    bool bInContact = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    TArray<FSofaDebugPoint> DebugPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    TArray<FSofaDebugPoint> CollisionDebugPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA")
    TArray<FSofaDebugTriangle> SurfaceTriangles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FSofaSurfaceMeshState SurfaceMesh;
};

USTRUCT(BlueprintType)
struct SOFABRIDGE_API FSofaRuntimeToolDescriptor
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FName ToolNodeName = TEXT("PrimaryTool");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FName CollisionNodeName = TEXT("PrimaryToolCollision");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FName ControlMechanicalObjectName = TEXT("controlMO");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FTransform InitialLocalTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    float SofaScale = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    FVector SofaScale3D = FVector(1.0, 1.0, 1.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    bool bVisible = true;
};

USTRUCT(BlueprintType)
struct FSofaToolState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName ToolId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FTransform UnrealLocalToolTransform = FTransform::Identity;

    UPROPERTY(BlueprintReadOnly)
    bool bValid = false;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA")
    TArray<FSofaToolState> Tools;
};

enum class ESofaBindingUsage : uint8
{
    ToolControl,
    ToolCollision,
    ObjectGoal,
    ConstraintTarget,
    AttachmentTarget,
    ObjectMechanical,
    ObjectSurface,
    ObjectVisual,
    Custom
};

struct FSofaMechanicalBindingDescriptor
{
    FName BindingId;
    FName OwnerId;
    ESofaBindingUsage Usage = ESofaBindingUsage::Custom;
    FString NodePath;
    FString ObjectKey;
    FString SurfaceTopologyObjectKey;
    FString VisualNodePath;
    FString VisualObjectKey;
    FString VisualTopologyObjectKey;
    FString CollisionNodePath;
    FString CollisionObjectKey;
};