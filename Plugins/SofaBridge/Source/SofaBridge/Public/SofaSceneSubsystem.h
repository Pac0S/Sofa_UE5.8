#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SofaSceneTypes.h"
#include "SofaSimulationService.h"
#include "SofaSceneSubsystem.generated.h"

UCLASS()
class SOFABRIDGE_API USofaSceneSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual ~USofaSceneSubsystem();

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void Tick(float DeltaTime);
    virtual TStatId GetStatId() const;

    UFUNCTION(BlueprintCallable, Category = "SOFA|Simulation")
    void ConfigurePrototypeScene(const FSofaPrototypeSceneRequest& Request);

    UFUNCTION(BlueprintCallable, Category="SOFA|Simulation")
    void StartSimulation();

    UFUNCTION(BlueprintCallable, Category="SOFA|Simulation")
    bool StartPrototypeSimulation();

    UFUNCTION(BlueprintCallable, Category="SOFA|Simulation")
    void StopPrototypeSimulation();

    UFUNCTION(BlueprintCallable, Category="SOFA|Simulation")
    void PauseSimulation();

    UFUNCTION(BlueprintCallable, Category="SOFA|Simulation")
    void ResumeSimulation();

    UFUNCTION(BlueprintCallable, Category = "SOFA|Simulation")
    void ClearPrototypeSceneRequest();

    UFUNCTION(BlueprintPure, Category = "SOFA|Simulation")
    bool HasPrototypeSceneRequest() const;

    UFUNCTION(BlueprintPure, Category = "SOFA|Simulation")
    FSofaPrototypeSceneRequest GetPrototypeSceneRequest() const;


    UFUNCTION(BlueprintCallable, Category="SOFA")
    bool TryGetLatestSnapshot(FSofaFrameSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    bool SetInteractorTargetPose(FName TargetId, const FTransform& TargetPose);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    bool ClearInteractorTargetPose(FName TargetId);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    bool GetObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const;

private:
    TUniquePtr<FSofaSimulationService> Service;

    bool bHasPrototypeSceneRequest = false;

    FSofaPrototypeSceneRequest PendingPrototypeSceneRequest;
};