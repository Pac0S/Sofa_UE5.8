#include "SofaProceduralSurfaceComponent.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SofaUtils.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogProceduralSurfaceComponent, Log, All);

USofaProceduralSurfaceComponent::USofaProceduralSurfaceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USofaProceduralSurfaceComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureProceduralMesh();
}

void USofaProceduralSurfaceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearMesh();
    Super::EndPlay(EndPlayReason);
}

void USofaProceduralSurfaceComponent::EnsureProceduralMesh()
{
    if (ProceduralMesh)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogProceduralSurfaceComponent, Warning, TEXT("EnsureProceduralMesh: no owner"));
        return;
    }

    ProceduralMesh = NewObject<UProceduralMeshComponent>(Owner, ProceduralMeshName);
    if (!ProceduralMesh)
    {
        UE_LOG(LogProceduralSurfaceComponent, Error, TEXT("EnsureProceduralMesh: failed to create UProceduralMeshComponent"));
        return;
    }

    ProceduralMesh->CreationMethod = EComponentCreationMethod::Instance;
    ProceduralMesh->RegisterComponent();

    if (USceneComponent* RootComp = Owner->GetRootComponent())
    {
        ProceduralMesh->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
    }
    else
    {
        Owner->SetRootComponent(ProceduralMesh);
    }

    ProceduralMesh->bUseComplexAsSimpleCollision = bCreateCollision;
    ProceduralMesh->SetCastShadow(bCastShadow);
    ProceduralMesh->SetCollisionEnabled(bCreateCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

    UE_LOG(LogProceduralSurfaceComponent, Log, TEXT("EnsureProceduralMesh: procedural mesh created for actor %s"),
        *Owner->GetName());
}

void USofaProceduralSurfaceComponent::ClearMesh()
{
    if (ProceduralMesh)
    {
        ProceduralMesh->ClearAllMeshSections();
    }
}

void USofaProceduralSurfaceComponent::SetMaterialPath(const FString& InMaterialPath)
{
    MaterialPath = InMaterialPath;
}

bool USofaProceduralSurfaceComponent::InitializeMaterial()
{
    if (!ProceduralMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeMaterial: ProceduralMesh is null"));
        return false;
    }

    if (MaterialPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeMaterial: MaterialPath is empty"));
        return false;
    }

    if (!BaseMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeMaterial: BaseMaterial is null"));
        return false;
    }

    SofaMaterialUtils::FSofaParsedMtl ParsedMtl;
    FString ParseError;
    if (!SofaMaterialUtils::ExtractMaterialDataFromMtl(MaterialPath, ParsedMtl, ParseError))
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeMaterial: failed to parse MTL '%s' : %s"),
            *MaterialPath, *ParseError);
        return false;
    }

    DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    if (!DynamicMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeMaterial: failed to create MID from BaseMaterial"));
        return false;
    }

    if (ParsedMtl.bHasKd)
    {
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColorTint"), ParsedMtl.Kd);
    }

    if (ParsedMtl.bHasKs)
    {
        DynamicMaterial->SetVectorParameterValue(TEXT("SpecularColor"), ParsedMtl.Ks);
    }

    if (ParsedMtl.bHasD)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), ParsedMtl.D);
    }

    if (ParsedMtl.bHasNi)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("RefractionIndex"), ParsedMtl.Ni);
    }

    if (ParsedMtl.bHasNs)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("Shininess"), ParsedMtl.Ns);
    }

    ProceduralMesh->SetMaterial(0, DynamicMaterial);

    UE_LOG(LogTemp, Log,
        TEXT("InitializeMaterial: applied MTL '%s' (newmtl='%s') to procedural mesh"),
        *MaterialPath,
        *ParsedMtl.MaterialName);

    return true;
}

void USofaProceduralSurfaceComponent::UpdateMeshFromSofaState(const FSofaObjectState& State)
{
    EnsureProceduralMesh();

    if (!ProceduralMesh)
    {
        UE_LOG(LogProceduralSurfaceComponent, Warning, TEXT("UpdateMeshFromSofaState: ProceduralMesh is null"));
        return;
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    const bool bOk = BuildProcMeshBuffers(State, Vertices, Triangles, Normals, UV0, VertexColors, Tangents);

    if (!bOk || Vertices.Num() == 0 || Triangles.Num() == 0)
    {
        UE_LOG(LogProceduralSurfaceComponent, Warning,
            TEXT("UpdateMeshFromSofaState: invalid mesh buffers for '%s' (Vertices=%d, Triangles=%d)"),
            *State.ObjectId.ToString(), Vertices.Num(), Triangles.Num());

        ProceduralMesh->ClearAllMeshSections();
        return;
    }

    ProceduralMesh->ClearAllMeshSections();

    ProceduralMesh->CreateMeshSection(
        0,
        Vertices,
        Triangles,
        Normals,
        UV0,
        VertexColors,
        Tangents,
        bCreateCollision);

    if (bDoubleSidedGeometry)
    {
        ProceduralMesh->SetMeshSectionVisible(0, true);
    }

    UE_LOG(LogProceduralSurfaceComponent, Verbose,
        TEXT("UpdateMeshFromSofaState: section updated for '%s' (Vertices=%d, Faces=%d)"),
        *State.ObjectId.ToString(),
        Vertices.Num(),
        Triangles.Num() / 3);
}

bool USofaProceduralSurfaceComponent::BuildProcMeshBuffers(
    const FSofaObjectState& State,
    TArray<FVector>& OutVertices,
    TArray<int32>& OutTriangles,
    TArray<FVector>& OutNormals,
    TArray<FVector2D>& OutUV0,
    TArray<FColor>& OutVertexColors,
    TArray<FProcMeshTangent>& OutTangents)
{
    OutVertices.Reset();
    OutTriangles.Reset();
    OutNormals.Reset();
    OutUV0.Reset();
    OutVertexColors.Reset();
    OutTangents.Reset();

    if (State.SurfaceMesh.Vertices.Num() == 0 || State.SurfaceTriangles.Num() == 0)
    {
        return false;
    }
    if(State.SurfaceMesh.Vertices.Num())

    OutVertices.Reserve(State.SurfaceMesh.Vertices.Num());
    OutUV0.Reserve(State.SurfaceMesh.Vertices.Num());
    OutVertexColors.Reserve(State.SurfaceMesh.Vertices.Num());

    for (const FVector& Pt : State.SurfaceMesh.Vertices)
    {
        OutVertices.Add(Pt);
        OutUV0.Add(FVector2D::ZeroVector);
        OutVertexColors.Add(FColor::White);
    }

    OutTriangles.Reserve(State.SurfaceMesh.Triangles.Num());

    for (int32 i = 0; i < State.SurfaceMesh.Triangles.Num(); i += 3)
    {
        if (!State.SurfaceMesh.Triangles.IsValidIndex(i)
            || !State.SurfaceMesh.Triangles.IsValidIndex(i + 1)
            || !State.SurfaceMesh.Triangles.IsValidIndex(i + 2))
        {
            UE_LOG(LogProceduralSurfaceComponent, Warning,
                TEXT("BuildProcMeshBuffers: invalid triangle indices at %d, skipping"), i);
            continue;
        }
        int32 triA = State.SurfaceMesh.Triangles[i];
        int32 triB = State.SurfaceMesh.Triangles[i + 1];
        int32 triC = State.SurfaceMesh.Triangles[i + 2];

        if(triA == triB || triB == triC || triC == triA)
        {
            UE_LOG(LogProceduralSurfaceComponent, Warning,
                TEXT("BuildProcMeshBuffers: degenerate triangle at %d, skipping"), i);
            continue;
        }
        OutTriangles.Add(triA);
        OutTriangles.Add(triB);
        OutTriangles.Add(triC);
    }

    if (OutTriangles.Num() < 3)
    {
        return false;
    }

    ComputeNormals(OutVertices, OutTriangles, OutNormals);
    ComputeTangents(OutNormals, OutTangents);

    return OutNormals.Num() == OutVertices.Num()
        && OutUV0.Num() == OutVertices.Num()
        && OutVertexColors.Num() == OutVertices.Num()
        && OutTangents.Num() == OutVertices.Num();
}

void USofaProceduralSurfaceComponent::ComputeNormals(
    const TArray<FVector>& Vertices,
    const TArray<int32>& Triangles,
    TArray<FVector>& OutNormals)
{
    OutNormals.Init(FVector::ZeroVector, Vertices.Num());

    for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
    {
        const int32 Ia = Triangles[i];
        const int32 Ib = Triangles[i + 1];
        const int32 Ic = Triangles[i + 2];

        if (!Vertices.IsValidIndex(Ia) ||
            !Vertices.IsValidIndex(Ib) ||
            !Vertices.IsValidIndex(Ic))
        {
            continue;
        }

        const FVector& A = Vertices[Ia];
        const FVector& B = Vertices[Ib];
        const FVector& C = Vertices[Ic];

        const FVector FaceNormal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();

        if (!FaceNormal.IsNearlyZero())
        {
            OutNormals[Ia] += FaceNormal;
            OutNormals[Ib] += FaceNormal;
            OutNormals[Ic] += FaceNormal;
        }
    }

    for (FVector& N : OutNormals)
    {
        N = N.GetSafeNormal();
        if (N.IsNearlyZero())
        {
            N = FVector::UpVector;
        }
    }
}

void USofaProceduralSurfaceComponent::ComputeTangents(
    const TArray<FVector>& Normals,
    TArray<FProcMeshTangent>& OutTangents)
{
    OutTangents.Reset();
    OutTangents.Reserve(Normals.Num());

    for (const FVector& N : Normals)
    {
        FVector TangentX = FVector::CrossProduct(FVector::UpVector, N);

        if (TangentX.IsNearlyZero())
        {
            TangentX = FVector::CrossProduct(FVector::RightVector, N);
        }

        TangentX = TangentX.GetSafeNormal();

        if (TangentX.IsNearlyZero())
        {
            TangentX = FVector::ForwardVector;
        }

        OutTangents.Add(FProcMeshTangent(TangentX, false));
    }
}