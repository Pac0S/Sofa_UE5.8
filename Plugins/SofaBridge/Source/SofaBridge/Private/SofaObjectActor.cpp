#include "SofaObjectActor.h"

#include "Components/SceneComponent.h"

ASofaObjectActor::ASofaObjectActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
}

void ASofaObjectActor::BeginPlay()
{
    Super::BeginPlay();
}

void ASofaObjectActor::InitializeFromObjectId(FName InObjectId)
{
    ObjectId = InObjectId;
    bInitialized = !ObjectId.IsNone();
}

void ASofaObjectActor::UpdateFromSofaState(const FSofaObjectState& State)
{
    if (!bInitialized)
    {
        InitializeFromObjectId(State.ObjectId);
    }
}

void ASofaObjectActor::SetObjectVisible(bool bVisible)
{
    SetActorHiddenInGame(!bVisible);
    SetActorEnableCollision(bVisible);
    SetActorTickEnabled(bVisible);
}
