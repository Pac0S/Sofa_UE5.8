#pragma once

#include "CoreMinimal.h"
#include "SofaObjectActor.h"
#include "SofaProceduralObjectActor.generated.h"

class USofaProceduralSurfaceComponent;

UCLASS()
class SOFABRIDGE_API ASofaProceduralObjectActor : public ASofaObjectActor
{
    GENERATED_BODY()

public:
    ASofaProceduralObjectActor();

    virtual void BeginPlay() override;
    virtual void InitializeFromObjectId(FName InObjectId) override;
    virtual void UpdateFromSofaState(const FSofaObjectState& State) override;
    virtual void SetObjectVisible(bool bVisible) override;

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void SetMaterialPath(const FString& InMaterialPath);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    bool InitializeMaterial();

    UFUNCTION(BlueprintPure, Category = "SOFA")
    USofaProceduralSurfaceComponent* GetProceduralSurfaceComponent() const { return ProceduralSurfaceComponent; }

    UFUNCTION(BlueprintCallable, Category = "SOFA|Material")
    void SetBaseMaterial(UMaterialInterface* InBaseMaterial);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<USofaProceduralSurfaceComponent> ProceduralSurfaceComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA")
    FString MaterialPath;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA|Material")
    TObjectPtr<UMaterialInterface> BaseMaterial = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA|Material")
    bool bMaterialInitialized = false;
};
