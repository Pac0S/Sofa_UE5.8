#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SofaRuntimeTypes.h"
#include "SofaSceneActor.generated.h"

class ASofaObjectActor;
class ASofaProceduralObjectActor;
class ASofaStaticObjectActor;
class ASofaToolActor;
class USceneComponent;
class USofaSceneSubsystem;

UCLASS()
class ASofaSceneActor : public AActor
{
    GENERATED_BODY()

public:
    ASofaSceneActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void SetSofaDebugVisible(bool bVisible);

protected:
    bool InitializeSubsystem();
    bool StartSimulation();
    void SyncSnapshot(const FSofaFrameSnapshot& Snapshot);
    void SyncObjects(const FSofaFrameSnapshot& Snapshot);
    void SyncTools(const FSofaFrameSnapshot& Snapshot);

    ASofaObjectActor* FindOrSpawnObjectActor(const FSofaObjectState& ObjState);
    ASofaToolActor* FindOrSpawnToolActor(FName ToolId);

    bool GetObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const;
    void DrawSofaDebug(const FSofaFrameSnapshot& Snapshot);
    void DrawDebugPointsActorSpace(const FSofaFrameSnapshot& Snapshot);
    void DrawDebugCollisionPointsActorSpace(const FSofaFrameSnapshot& Snapshot);
    void DrawDebugSurfaceActorSpace(const FSofaFrameSnapshot& Snapshot);
    void DrawStaticCollisionDebugPoints() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<USofaSceneSubsystem> SofaSubsystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Config")
    bool bAutoStartSimulation = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Config")
    bool bUseExplicitSceneFilePath = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Config")
    FString SceneFilePath;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Config")
    FString SceneName = TEXT("liver");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Config")
    FString ExternalScenesDirectory = TEXT("C:/Users/Pakito/Documents/Projets/Anisim/Sofa_UE_5_8_test/SofaScenes");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Config")
    FString RelativeScenesDirectory = TEXT("SofaScenes");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Objects")
    TSubclassOf<ASofaProceduralObjectActor> ProceduralObjectActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Objects")
    TSubclassOf<ASofaStaticObjectActor> StaticObjectActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Tools")
    TSubclassOf<ASofaToolActor> ToolActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Tools")
    FName DefaultToolId = TEXT("TOOL_Primary");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Debug")
    bool bShowSofaDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Material")
    TObjectPtr<UMaterialInterface> DefaultProceduralBaseMaterial = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TMap<FName, TObjectPtr<ASofaObjectActor>> ObjectActorsById;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TMap<FName, TObjectPtr<ASofaToolActor>> ToolActorsById;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA|Debug")
    bool bCaptureStaticDebugPointsOnce = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA|Debug")
    bool bStaticDebugPointsCaptured = false;

    TMap<FName, TArray<FSofaDebugPoint>> StaticCollisionDebugPointsByMesh;

    void LogSceneTransform() const;
    void LogObjectActorTransform(const FName ObjectId) const;
    void LogProceduralMeshTransform(const FName ObjectId) const;
    void LogTransformChain(const FName ObjectId) const;
    void LogFirstPointAndVertex(const FSofaObjectState& Obj) const;

};
