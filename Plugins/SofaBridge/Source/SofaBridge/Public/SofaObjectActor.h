#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SofaRuntimeTypes.h"
#include "SofaObjectActor.generated.h"

UCLASS(Abstract)
class ASofaObjectActor : public AActor
{
    GENERATED_BODY()

public:
    ASofaObjectActor();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    virtual void InitializeFromObjectId(FName InObjectId);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    virtual void UpdateFromSofaState(const FSofaObjectState& State);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    virtual void SetObjectVisible(bool bVisible);

    UFUNCTION(BlueprintPure, Category = "SOFA")
    FName GetObjectId() const { return ObjectId; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA")
    FName ObjectId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    bool bInitialized = false;
};
