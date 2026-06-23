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

    UFUNCTION(BlueprintCallable, Category="SOFA")
    void StartPrototypeSimulation();

    UFUNCTION(BlueprintCallable, Category="SOFA")
    void StopPrototypeSimulation();

    UFUNCTION(BlueprintCallable, Category="SOFA")
    void PauseSimulation();

    UFUNCTION(BlueprintCallable, Category="SOFA")
    void ResumeSimulation();

private:
    TUniquePtr<FSofaSimulationService> Service;
};