#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SofaRuntimeTypes.h"
#include "SofaToolActor.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USofaSceneSubsystem;

UCLASS()
class ASofaToolActor : public AActor
{
    GENERATED_BODY()

public:
    ASofaToolActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void InitializeToolActor(USofaSceneSubsystem* InSofaSubsystem, FName InToolId);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void SetDesiredLocalTransform(const FTransform& InLocalTransform);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void SetStaticMesh(UStaticMesh* InStaticMesh);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void SubmitCurrentToolInput();

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void RefreshFromSnapshot(const FSofaFrameSnapshot& Snapshot);

    UFUNCTION(BlueprintPure, Category = "SOFA")
    FTransform GetDesiredLocalTransform() const { return DesiredLocalTransform; }

    UFUNCTION(BlueprintPure, Category = "SOFA")
    FTransform GetCurrentSimulatedLocalTransform() const { return CurrentSimulatedLocalTransform; }

    UFUNCTION(BlueprintPure, Category = "SOFA")
    FName GetToolId() const { return ToolId; }

    UFUNCTION(BlueprintPure, Category = "SOFA")
    bool HasValidSnapshotPose() const { return bHasSnapshotPose; }

protected:
    void UpdateKeyboardControl(float DeltaSeconds);
    void DrawToolDebug() const;

    FTransform ToolWorldToLocalTransform(const FTransform& WorldTransform) const;
    FTransform ToolLocalToWorldTransform(const FTransform& LocalTransform) const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<USphereComponent> RootSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY()
    TObjectPtr<USofaSceneSubsystem> SofaSubsystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA")
    FName ToolId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    FTransform InitialSpawnWorldTransform;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    FTransform DesiredLocalTransform;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    FTransform CurrentSimulatedLocalTransform;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    FTransform LastSubmittedLocalTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA|Input")
    float InputMoveSpeed = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA|Input")
    bool bDriveFromKeyboard = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA|Debug")
    bool bFollowSnapshot = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA|Debug")
    bool bDrawDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA|Debug")
    float DebugSphereRadius = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA|Debug")
    float DebugAxisLength = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA|Debug")
    float DebugTextZOffset = 12.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    bool bInitialized = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    bool bHasSnapshotPose = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    bool bHasReceivedInitialSnapshot = false;
};
