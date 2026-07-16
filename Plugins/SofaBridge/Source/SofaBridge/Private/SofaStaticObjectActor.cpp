#include "SofaStaticObjectActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ASofaStaticObjectActor::ASofaStaticObjectActor()
{
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
    StaticMeshComponent->SetupAttachment(GetRootComponent());
    StaticMeshComponent->SetMobility(EComponentMobility::Movable);
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StaticMeshComponent->SetGenerateOverlapEvents(false);
}

void ASofaStaticObjectActor::InitializeFromObjectId(FName InObjectId)
{
    Super::InitializeFromObjectId(InObjectId);
}

void ASofaStaticObjectActor::UpdateFromSofaState(const FSofaObjectState& State)
{
    Super::UpdateFromSofaState(State);
}

void ASofaStaticObjectActor::SetObjectVisible(bool bVisible)
{
    Super::SetObjectVisible(bVisible);

    if (StaticMeshComponent)
    {
        StaticMeshComponent->SetVisibility(bVisible, true);
    }
}

void ASofaStaticObjectActor::SetStaticMesh(UStaticMesh* InStaticMesh)
{
    if (StaticMeshComponent)
    {
        StaticMeshComponent->SetStaticMesh(InStaticMesh);
    }
}

void ASofaStaticObjectActor::SetMaterial(int32 MaterialIndex, UMaterialInterface* InMaterial)
{
    if (StaticMeshComponent)
    {
        StaticMeshComponent->SetMaterial(MaterialIndex, InMaterial);
    }
}
