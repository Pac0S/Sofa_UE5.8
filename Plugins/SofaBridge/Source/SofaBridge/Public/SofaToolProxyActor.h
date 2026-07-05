#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SofaToolProxyActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USofaSceneSubsystem;

UCLASS()
class SOFABRIDGE_API ASofaToolProxyActor : public AActor
{
    GENERATED_BODY()

public:
    ASofaToolProxyActor();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    void InitializeToolProxy(USofaSceneSubsystem* InSubsystem, FName InToolId);

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent> RootSphere = nullptr;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

    UPROPERTY()
    TObjectPtr<USofaSceneSubsystem> SofaSubsystem = nullptr;

    UPROPERTY(EditAnywhere, Category="SOFA|Tool")
    FName ToolId = NAME_None;

    UPROPERTY(EditAnywhere, Category="SOFA|Tool")
    float InputMoveSpeed = 150.0f;

    UPROPERTY(EditAnywhere, Category="SOFA|Tool")
    bool bDriveFromKeyboard = true;

    UPROPERTY(EditAnywhere, Category="SOFA|Tool")
    bool bFollowSnapshot = true;

    UPROPERTY(EditAnywhere, Category="SOFA|Debug")
    bool bDrawDebug = true;

    UPROPERTY(EditAnywhere, Category="SOFA|Debug")
    float DebugSphereRadius = 6.0f;

    UPROPERTY(EditAnywhere, Category="SOFA|Debug")
    float DebugAxisLength = 20.0f;

    UPROPERTY(EditAnywhere, Category="SOFA|Debug")
    float DebugTextZOffset = 12.0f;

private:
    bool bInitialized = false;
    bool bHasSnapshotPose = false;
    FTransform LastSnapshotTransform = FTransform::Identity;

    void UpdateKeyboardControl(float DeltaSeconds);
    void SubmitCurrentToolInput();
    void RefreshFromSnapshot();
    void DrawToolDebug();
};