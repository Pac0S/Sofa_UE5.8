// SofaTestActor.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SofaTestActor.generated.h"

UCLASS()
class SOFABRIDGE_API ASofaTestActor : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};