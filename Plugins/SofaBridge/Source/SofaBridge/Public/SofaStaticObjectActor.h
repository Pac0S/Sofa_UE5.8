#pragma once

#include "CoreMinimal.h"
#include "SofaObjectActor.h"
#include "SofaStaticObjectActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class SOFABRIDGE_API ASofaStaticObjectActor : public ASofaObjectActor
{
    GENERATED_BODY()

public:
    ASofaStaticObjectActor();

    virtual void InitializeFromObjectId(FName InObjectId) override;
    virtual void UpdateFromSofaState(const FSofaObjectState& State) override;
    virtual void SetObjectVisible(bool bVisible) override;

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void SetStaticMesh(UStaticMesh* InStaticMesh);

    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void SetMaterial(int32 MaterialIndex, UMaterialInterface* InMaterial);

    UFUNCTION(BlueprintPure, Category = "SOFA")
    UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<UStaticMeshComponent> StaticMeshComponent;
};
