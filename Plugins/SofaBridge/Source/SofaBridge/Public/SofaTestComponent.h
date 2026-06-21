#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SofaTestComponent.generated.h"

/**
 * Composant de test pour vérifier l'intégration SOFA (SPtr, namespaces, SimpleApi)
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOFABRIDGE_API USofaTestComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USofaTestComponent();

protected:
    virtual void BeginPlay() override;

public:
    // Lance un test simple SOFA au début
    UFUNCTION(BlueprintCallable, Category = "SOFA")
    void RunSofaTest();
};