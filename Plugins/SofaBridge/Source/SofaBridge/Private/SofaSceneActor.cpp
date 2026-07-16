#include "SofaSceneActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

#include "ProceduralMeshComponent.h"
#include "SofaProceduralSurfaceComponent.h"

#include "SofaObjectActor.h"
#include "SofaProceduralObjectActor.h"
#include "SofaStaticObjectActor.h"
#include "SofaToolActor.h"
#include "SofaSceneSubsystem.h"


ASofaSceneActor::ASofaSceneActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
}

void ASofaSceneActor::BeginPlay()
{
    Super::BeginPlay();

    if (!bAutoStartSimulation)
    {
        return;
    }

    if (!InitializeSubsystem())
    {
        return;
    }

    StartSimulation();
}

void ASofaSceneActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!SofaSubsystem)
    {
        return;
    }

    FSofaFrameSnapshot Snapshot;
    if (!SofaSubsystem->TryGetLatestSnapshot(Snapshot))
    {
        return;
    }

    SyncSnapshot(Snapshot);

    if (bCaptureStaticDebugPointsOnce && !bStaticDebugPointsCaptured)
    {
        TMap<FName, TArray<FSofaDebugPoint>> RetrievedPointsByMesh;
        FString ErrorMessage;

        if (SofaSubsystem->GetStaticCollisionDebugPointsByMesh(RetrievedPointsByMesh, ErrorMessage))
        {
            StaticCollisionDebugPointsByMesh = MoveTemp(RetrievedPointsByMesh);
            bStaticDebugPointsCaptured = true;

            UE_LOG(LogTemp, Log, TEXT("[SOFA][Debug] Static collision debug points captured once. MeshCount=%d"), StaticCollisionDebugPointsByMesh.Num());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SOFA][Debug] Failed to capture static collision debug points: %s"), *ErrorMessage);
        }
    }

    if (bShowSofaDebug)
    {
        DrawSofaDebug(Snapshot);
    }
}

void ASofaSceneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SofaSubsystem)
    {
        SofaSubsystem->StopPrototypeSimulation();
        SofaSubsystem = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void ASofaSceneActor::SetSofaDebugVisible(bool bVisible)
{
    bShowSofaDebug = bVisible;
}

bool ASofaSceneActor::InitializeSubsystem()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    SofaSubsystem = World->GetSubsystem<USofaSceneSubsystem>();
    return (SofaSubsystem != nullptr);
}

bool ASofaSceneActor::StartSimulation()
{
    if (!SofaSubsystem)
    {
        return false;
    }

    FSofaPrototypeSceneRequest Request;
    Request.bUseSceneFilePath = bUseExplicitSceneFilePath;
    Request.SceneFilePath = SceneFilePath;
    Request.SceneName = SceneName;
    Request.ExternalScenesDirectory = ExternalScenesDirectory;
    Request.RelativeScenesDirectory = RelativeScenesDirectory;

    SofaSubsystem->ConfigurePrototypeScene(Request);
    return SofaSubsystem->StartPrototypeSimulation();
}

void ASofaSceneActor::SyncSnapshot(const FSofaFrameSnapshot& Snapshot)
{
    SyncObjects(Snapshot);
    SyncTools(Snapshot);
}

void ASofaSceneActor::SyncObjects(const FSofaFrameSnapshot& Snapshot)
{
    for (const FSofaObjectState& ObjState : Snapshot.Objects)
    {
        ASofaObjectActor* ObjectActor = FindOrSpawnObjectActor(ObjState);
        if (!ObjectActor)
        {
            continue;
        }

        ObjectActor->UpdateFromSofaState(ObjState);
        /*if (ObjState.ObjectId == FName(TEXT("OBJ_Liver")))
        {
            LogTransformChain(ObjState.ObjectId);
            LogFirstPointAndVertex(ObjState);
        }*/
    }
}

void ASofaSceneActor::SyncTools(const FSofaFrameSnapshot& Snapshot)
{
    if (Snapshot.Tools.Num() == 0 && !DefaultToolId.IsNone())
    {
        FindOrSpawnToolActor(DefaultToolId);
        return;
    }

    for (const FSofaToolState& ToolState : Snapshot.Tools)
    {
        ASofaToolActor* ToolActor = FindOrSpawnToolActor(ToolState.ToolId);
        if (!ToolActor)
        {
            continue;
        }

        ToolActor->RefreshFromSnapshot(Snapshot);
    }
}

ASofaObjectActor* ASofaSceneActor::FindOrSpawnObjectActor(const FSofaObjectState& ObjState)
{
    if (TObjectPtr<ASofaObjectActor>* Found = ObjectActorsById.Find(ObjState.ObjectId))
    {
        return Found->Get();
    }

    if (!SofaSubsystem)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FSofaRuntimeObjectDescriptor ObjectDesc;
    SofaSubsystem->FindRuntimeObjectDescriptor(ObjectDesc, ObjState.ObjectId);

    if (ObjectDesc.ObjectNodeName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("FindOrSpawnObjectActor: no runtime descriptor found for ObjectId '%s'"),
            *ObjState.ObjectId.ToString());
        return nullptr;
    }

    UClass* ClassToSpawn = nullptr;

    switch (ObjectDesc.Role)
    {
    case ESofaRuntimeObjectRole::Organ:
    case ESofaRuntimeObjectRole::DeformableLayer:
        ClassToSpawn = ProceduralObjectActorClass
            ? ProceduralObjectActorClass.Get()
            : ASofaProceduralObjectActor::StaticClass();
        break;

    case ESofaRuntimeObjectRole::StaticSupport:
        ClassToSpawn = StaticObjectActorClass
            ? StaticObjectActorClass.Get()
            : ASofaStaticObjectActor::StaticClass();
        break;

    case ESofaRuntimeObjectRole::CollisionProxy:
        UE_LOG(LogTemp, Verbose,
            TEXT("FindOrSpawnObjectActor: skipping CollisionProxy '%s'"),
            *ObjState.ObjectId.ToString());
        return nullptr;

    case ESofaRuntimeObjectRole::Tool:
        UE_LOG(LogTemp, Warning,
            TEXT("FindOrSpawnObjectActor: '%s' resolved as Tool; should be handled by tool pipeline"),
            *ObjState.ObjectId.ToString());
        return nullptr;

    case ESofaRuntimeObjectRole::Unknown:
    default:
        UE_LOG(LogTemp, Warning,
            TEXT("FindOrSpawnObjectActor: unsupported role for '%s'"),
            *ObjState.ObjectId.ToString());
        return nullptr;
    }

    if (!ClassToSpawn)
    {
        return nullptr;
    }

    const FTransform SpawnWorldTransform = GetActorTransform();

    ASofaObjectActor* Spawned = World->SpawnActorDeferred<ASofaObjectActor>(ClassToSpawn, SpawnWorldTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Spawned)
    {
        return nullptr;
    }
    Spawned->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
    Spawned->SetActorRelativeTransform(FTransform::Identity);
    Spawned->InitializeFromObjectId(ObjState.ObjectId);
    Spawned->SetObjectVisible(ObjectDesc.bVisible);

    if (ASofaProceduralObjectActor* ProceduralActor = Cast<ASofaProceduralObjectActor>(Spawned))
    {
        if (DefaultProceduralBaseMaterial)
        {
            ProceduralActor->SetBaseMaterial(DefaultProceduralBaseMaterial);
        }

        if (!ObjectDesc.VisualMaterialPath.IsEmpty())
        {
            ProceduralActor->SetMaterialPath(ObjectDesc.VisualMaterialPath);
        }
    }
    else if (ASofaStaticObjectActor* StaticActor = Cast<ASofaStaticObjectActor>(Spawned))
    {
        if (!ObjectDesc.StaticMeshPath.IsEmpty())
        {
            if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectDesc.StaticMeshPath))
            {
                FVector Scale3D = FVector(ObjectDesc.SofaScale * ObjectDesc.SofaScale3D.X, ObjectDesc.SofaScale * ObjectDesc.SofaScale3D.Z, ObjectDesc.SofaScale * ObjectDesc.SofaScale3D.Y);
                Spawned->SetActorRelativeTransform(ObjectDesc.InitialLocalTransform);
                StaticActor->SetStaticMesh(Mesh);
                Spawned->SetActorScale3D(Scale3D);
            }
        }
    }

    Spawned->FinishSpawning(SpawnWorldTransform);

    ObjectActorsById.Add(ObjState.ObjectId, Spawned);
    return Spawned;
}

ASofaToolActor* ASofaSceneActor::FindOrSpawnToolActor(FName InToolId)
{
    if (TObjectPtr<ASofaToolActor>* Found = ToolActorsById.Find(InToolId))
    {
        return Found->Get();
    }

    if (!SofaSubsystem)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FSofaRuntimeToolDescriptor ToolDesc;
    SofaSubsystem->FindRuntimeToolDescriptor(ToolDesc, InToolId);

    if (ToolDesc.ToolNodeName.IsNone())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("FindOrSpawnToolActor: no runtime descriptor found for ToolId '%s'"),
            *InToolId.ToString());
        return nullptr;
    }

    UClass* ClassToSpawn = ToolActorClass? ToolActorClass.Get() : ASofaToolActor::StaticClass();

    if (!ClassToSpawn)
    {
        return nullptr;
    }

    const FTransform SpawnWorldTransform = GetActorTransform();

    ASofaToolActor* Spawned = World->SpawnActorDeferred<ASofaToolActor>(ClassToSpawn, SpawnWorldTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Spawned)
    {
        return nullptr;
    }

    Spawned->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
    Spawned->SetActorRelativeTransform(FTransform::Identity);
    Spawned->InitializeToolActor(SofaSubsystem, InToolId);

    Spawned->FinishSpawning(SpawnWorldTransform);

    ToolActorsById.Add(InToolId, Spawned);
    return Spawned;
}

bool ASofaSceneActor::GetObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const
{
    if (!SofaSubsystem)
    {
        return false;
    }

    return SofaSubsystem->GetObjectMaterialPath(ObjectId, OutMaterialPath);
}

void ASofaSceneActor::DrawSofaDebug(const FSofaFrameSnapshot& Snapshot)
{
    DrawDebugPointsActorSpace(Snapshot);
    DrawDebugSurfaceActorSpace(Snapshot);
    DrawDebugCollisionPointsActorSpace(Snapshot);
    DrawStaticCollisionDebugPoints();
}

void ASofaSceneActor::DrawDebugPointsActorSpace(const FSofaFrameSnapshot& Snapshot)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform SceneToWorld = GetActorTransform();

    for (const FSofaObjectState& Obj : Snapshot.Objects)
    {
        for (const FSofaDebugPoint& Pt : Obj.DebugPoints)
        {
            const FVector WorldPos = SceneToWorld.TransformPosition(Pt.Position);
            DrawDebugPoint(World, WorldPos, Pt.Size, Pt.Color, false, 0.0f);
        }
    }
}

void ASofaSceneActor::DrawDebugCollisionPointsActorSpace(const FSofaFrameSnapshot& Snapshot)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform SceneToWorld = GetActorTransform();

    for (const FSofaObjectState& Obj : Snapshot.Objects)
    {
        for (const FSofaDebugPoint& Pt : Obj.CollisionDebugPoints)
        {
            const FVector WorldPos = SceneToWorld.TransformPosition(Pt.Position);
            DrawDebugPoint(World, WorldPos, Pt.Size, Pt.Color, false, 0.0f);
        }
    }
}

void ASofaSceneActor::DrawDebugSurfaceActorSpace(const FSofaFrameSnapshot& Snapshot)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform SceneToWorld = GetActorTransform();

    for (const FSofaObjectState& Obj : Snapshot.Objects)
    {
        for (const FSofaDebugTriangle& T : Obj.SurfaceTriangles)
        {
            if (!Obj.DebugPoints.IsValidIndex(T.A) ||
                !Obj.DebugPoints.IsValidIndex(T.B) ||
                !Obj.DebugPoints.IsValidIndex(T.C))
            {
                continue;
            }

            const FVector WorldA = SceneToWorld.TransformPosition(Obj.DebugPoints[T.A].Position);
            const FVector WorldB = SceneToWorld.TransformPosition(Obj.DebugPoints[T.B].Position);
            const FVector WorldC = SceneToWorld.TransformPosition(Obj.DebugPoints[T.C].Position);

            DrawDebugLine(World, WorldA, WorldB, FColor::Cyan, false, 0.f, 0, 0.5f);
            DrawDebugLine(World, WorldB, WorldC, FColor::Cyan, false, 0.f, 0, 0.5f);
            DrawDebugLine(World, WorldC, WorldA, FColor::Cyan, false, 0.f, 0, 0.5f);
        }
    }
}

void ASofaSceneActor::DrawStaticCollisionDebugPoints() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform ActorTransform = GetActorTransform();

    for (const TPair<FName, TArray<FSofaDebugPoint>>& Pair : StaticCollisionDebugPointsByMesh)
    {
        const FName MeshName = Pair.Key;
        const TArray<FSofaDebugPoint>& Points = Pair.Value;

        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            const FSofaDebugPoint& DebugPoint = Points[Index];
            const FVector WorldPosition = ActorTransform.TransformPosition(DebugPoint.Position);

            DrawDebugPoint(World, WorldPosition, DebugPoint.Size, DebugPoint.Color, false, 0.0f, 0);
        }

        UE_LOG(
            LogTemp,
            VeryVerbose,
            TEXT("[SOFA][Debug] Drew %d static collision debug points for mesh '%s'."),
            Points.Num(),
            *MeshName.ToString());
    }
}

static FString SofaTransformToString(const FTransform& Xf)
{
    const FVector T = Xf.GetLocation();
    const FRotator R = Xf.Rotator();
    const FVector S = Xf.GetScale3D();

    return FString::Printf(
        TEXT("Loc=(%.3f, %.3f, %.3f) Rot=(P=%.3f Y=%.3f R=%.3f) Scale=(%.3f, %.3f, %.3f)"),
        T.X, T.Y, T.Z,
        R.Pitch, R.Yaw, R.Roll,
        S.X, S.Y, S.Z);
}

void ASofaSceneActor::LogSceneTransform() const
{
    UE_LOG(LogTemp, Warning,
        TEXT("[SOFA][SceneActor] %s"),
        *SofaTransformToString(GetActorTransform()));
}

void ASofaSceneActor::LogObjectActorTransform(const FName ObjectId) const
{
    const TObjectPtr<ASofaObjectActor>* FoundActorPtr = ObjectActorsById.Find(ObjectId);
    if (!FoundActorPtr || !FoundActorPtr->Get())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SOFA][ObjectActor] ObjectId='%s' not found"),
            *ObjectId.ToString());
        return;
    }

    const ASofaObjectActor* ObjectActor = FoundActorPtr->Get();

    UE_LOG(LogTemp, Warning,
        TEXT("[SOFA][ObjectActor] ObjectId='%s' Actor='%s' %s"),
        *ObjectId.ToString(),
        *ObjectActor->GetName(),
        *SofaTransformToString(ObjectActor->GetActorTransform()));
}

void ASofaSceneActor::LogProceduralMeshTransform(const FName ObjectId) const
{
    const TObjectPtr<ASofaObjectActor>* FoundActorPtr = ObjectActorsById.Find(ObjectId);
    if (!FoundActorPtr || !FoundActorPtr->Get())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SOFA][ProcMesh] ObjectId='%s' actor not found"),
            *ObjectId.ToString());
        return;
    }

    const ASofaProceduralObjectActor* ProceduralActor = Cast<ASofaProceduralObjectActor>(FoundActorPtr->Get());
    if (!ProceduralActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SOFA][ProcMesh] ObjectId='%s' actor is not ASofaProceduralObjectActor"),
            *ObjectId.ToString());
        return;
    }

    const USofaProceduralSurfaceComponent* SurfaceComp = ProceduralActor->GetProceduralSurfaceComponent();
    if (!SurfaceComp)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SOFA][ProcMesh] ObjectId='%s' no SurfaceComponent"),
            *ObjectId.ToString());
        return;
    }

    const UProceduralMeshComponent* ProcMesh = SurfaceComp->GetProceduralMeshComponent();
    if (!ProcMesh)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SOFA][ProcMesh] ObjectId='%s' no ProceduralMeshComponent"),
            *ObjectId.ToString());
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SOFA][ProcMesh] ObjectId='%s' Component='%s' WORLD=%s REL=%s"),
        *ObjectId.ToString(),
        *ProcMesh->GetName(),
        *SofaTransformToString(ProcMesh->GetComponentTransform()),
        *SofaTransformToString(ProcMesh->GetRelativeTransform()));
}

void ASofaSceneActor::LogTransformChain(const FName ObjectId) const
{
    UE_LOG(LogTemp, Warning, TEXT("========== SOFA TRANSFORM CHAIN '%s' =========="), *ObjectId.ToString());
    LogSceneTransform();
    LogObjectActorTransform(ObjectId);
    LogProceduralMeshTransform(ObjectId);
    UE_LOG(LogTemp, Warning, TEXT("================================================"));
}

void ASofaSceneActor::LogFirstPointAndVertex(const FSofaObjectState& Obj) const
{
    if (Obj.DebugPoints.Num() == 0 || Obj.SurfaceMesh.Vertices.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SOFA][Geom] Object '%s' has insufficient debug/mesh data (Points=%d, Vertices=%d)"),
            *Obj.ObjectId.ToString(),
            Obj.DebugPoints.Num(),
            Obj.SurfaceMesh.Vertices.Num());
        return;
    }

    const FVector DebugLocal = Obj.DebugPoints[0].Position;
    const FVector MeshLocal = Obj.SurfaceMesh.Vertices[0];

    const FTransform SceneToWorld = GetActorTransform();
    const FVector DebugWorld = SceneToWorld.TransformPosition(DebugLocal);
    const FVector MeshWorld = SceneToWorld.TransformPosition(MeshLocal);

    UE_LOG(LogTemp, Warning,
        TEXT("[SOFA][Geom] '%s' DebugLocal=%s MeshLocal=%s"),
        *Obj.ObjectId.ToString(),
        *DebugLocal.ToString(),
        *MeshLocal.ToString());

    UE_LOG(LogTemp, Warning,
        TEXT("[SOFA][Geom] '%s' DebugWorld=%s MeshWorld=%s"),
        *Obj.ObjectId.ToString(),
        *DebugWorld.ToString(),
        *MeshWorld.ToString());
}