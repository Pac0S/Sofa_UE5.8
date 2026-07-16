#include "SofaProceduralObjectActor.h"
#include "SofaProceduralSurfaceComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

ASofaProceduralObjectActor::ASofaProceduralObjectActor()
{
    ProceduralSurfaceComponent = CreateDefaultSubobject<USofaProceduralSurfaceComponent>(TEXT("ProceduralSurfaceComponent"));
}

void ASofaProceduralObjectActor::BeginPlay()
{
    Super::BeginPlay();

    if (!MaterialPath.IsEmpty())
    {
        ProceduralSurfaceComponent->SetMaterialPath(MaterialPath);
        ProceduralSurfaceComponent->InitializeMaterial();
    }
}

void ASofaProceduralObjectActor::InitializeFromObjectId(FName InObjectId)
{
    Super::InitializeFromObjectId(InObjectId);
}

void ASofaProceduralObjectActor::UpdateFromSofaState(const FSofaObjectState& State)
{
    Super::UpdateFromSofaState(State);

    if (!bMaterialInitialized && ProceduralSurfaceComponent)
    {
        bMaterialInitialized = ProceduralSurfaceComponent->InitializeMaterial();
    }

    if (ProceduralSurfaceComponent)
    {
        ProceduralSurfaceComponent->UpdateMeshFromSofaState(State);
    }
}

void ASofaProceduralObjectActor::SetObjectVisible(bool bVisible)
{
    Super::SetObjectVisible(bVisible);

    if (ProceduralSurfaceComponent)
    {
        ProceduralSurfaceComponent->SetVisibility(bVisible, true);
    }
}

void ASofaProceduralObjectActor::SetMaterialPath(const FString& InMaterialPath)
{
    MaterialPath = InMaterialPath;

    if (ProceduralSurfaceComponent)
    {
        ProceduralSurfaceComponent->SetMaterialPath(MaterialPath);
    }
}

void ASofaProceduralObjectActor::SetBaseMaterial(UMaterialInterface* InBaseMaterial)
{
    BaseMaterial = InBaseMaterial;

    if (ProceduralSurfaceComponent)
    {
        ProceduralSurfaceComponent->SetBaseMaterial(BaseMaterial);
    }
}

bool ASofaProceduralObjectActor::InitializeMaterial()
{
    if (!ProceduralSurfaceComponent)
    {
        return false;
    }

    if (!MaterialPath.IsEmpty())
    {
        ProceduralSurfaceComponent->SetMaterialPath(MaterialPath);
    }

    return ProceduralSurfaceComponent->InitializeMaterial();
}
