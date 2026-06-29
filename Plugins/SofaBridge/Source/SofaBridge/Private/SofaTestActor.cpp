// SofaTestActor.cpp
#include "SofaTestActor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


ASofaTestActor::ASofaTestActor()
{
    PrimaryActorTick.bCanEverTick = true;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    ProceduralSurfaceComponent = CreateDefaultSubobject<USofaProceduralSurfaceComponent>(TEXT("ProceduralSurfaceComponent"));
}

void ASofaTestActor::BeginPlay()
{
    Super::BeginPlay();

    if (!bAutoStartSimulation)
    {
        UE_LOG(LogTemp, Log, TEXT("SOFA test actor: auto start disabled."));
        return;
    }

    SofaSubsystem = GetWorld()->GetSubsystem<USofaSceneSubsystem>();
    if (!SofaSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("SOFA test actor: failed to get USofaSceneSubsystem."));
        return;
        
    }

    FSofaPrototypeSceneRequest Request;
    Request.bUseSceneFilePath = bUseExplicitSceneFilePath;
    Request.SceneFilePath = SceneFilePath;
    Request.SceneName = SceneName;
    Request.ExternalScenesDirectory = ExternalScenesDirectory;
    Request.RelativeScenesDirectory = RelativeScenesDirectory;

    SofaSubsystem->ConfigurePrototypeScene(Request);

    const bool bStarted = SofaSubsystem->StartPrototypeSimulation();
    UE_LOG(LogTemp, Log, TEXT("SOFA test actor: StartPrototypeSimulation returned %s"),
        bStarted ? TEXT("true") : TEXT("false"));

    //MaterialPath only exists after SofaSceneBuilder construction that is called through StartPrototypeSimulation.
    FString MaterialPath;
    GetObjectMaterialPath(FName("Liver01"), MaterialPath);
    ProceduralSurfaceComponent->SetMaterialPath(MaterialPath);
    ProceduralSurfaceComponent->InitializeMaterial();
}

void ASofaTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SofaSubsystem)
    {
        SofaSubsystem->StopPrototypeSimulation();
        SofaSubsystem = nullptr;
        UE_LOG(LogTemp, Log, TEXT("SofaTestActor: simulation ended"));
    }
    Super::EndPlay(EndPlayReason);
}

void ASofaTestActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ProceduralSurfaceComponent)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (!SofaSubsystem)
    {
        return;
    }

    FSofaFrameSnapshot Snapshot;
    if (!SofaSubsystem->TryGetLatestSnapshot(Snapshot))
    {
        return;
    }

    for (const FSofaObjectState& ObjState : Snapshot.Objects)
    {
        UE_LOG(LogTemp, Verbose, TEXT("SofaTestActor: updating PMC for %s with %d points and %d triangles"),
            *ObjState.ObjectId.ToString(), ObjState.DebugPoints.Num(), ObjState.SurfaceTriangles.Num());
        if (ObjState.ObjectId == VisualizedObjectId)
        {
            ProceduralSurfaceComponent->UpdateMeshFromSofaState(ObjState);
        }
        if (bShowSofaDebug)
        {
            DrawSofaDebug(ObjState);
        }
    }

    // Apply periodic force if enabled
    if (bEnablePeriodicMotion && SofaSubsystem)
    {
        const float T = GetWorld()->GetTimeSeconds();
        const float Offset = MotionAmplitudeCm * FMath::Sin(2.f * PI * MotionFrequencyHz * T);

        FTransform Target = MotionBaseTransform;
        Target.AddToTranslation(MotionAxis.GetSafeNormal() * Offset);

        SofaSubsystem->SetInteractorTargetPose(MotionTargetId, Target);
    }
    else if (SofaSubsystem)
    {
        SofaSubsystem->ClearInteractorTargetPose(MotionTargetId);
    }
}

void ASofaTestActor::SetSofaDebugVisible(bool bVisible)
{
    bShowSofaDebug = bVisible;
}

bool ASofaTestActor::GetObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const
{
    if (!SofaSubsystem)
    {
        return false;
    }
    return SofaSubsystem->GetObjectMaterialPath(ObjectId, OutMaterialPath);
}

void ASofaTestActor::DrawSofaDebug(const FSofaObjectState& ObjState)
{
    DrawDebugPointsActorSpace(ObjState);
    DrawDebugSurfaceActorSpace(ObjState);
}

void ASofaTestActor::DrawDebugPointsActorSpace(const FSofaObjectState& ObjState)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    FSofaFrameSnapshot Snapshot;
    if (!SofaSubsystem->TryGetLatestSnapshot(Snapshot))
    {
        return;
    }
    for (const FSofaObjectState& Obj : Snapshot.Objects)
    {
        for (const FSofaDebugPoint& Pt : Obj.DebugPoints)
        {
            const FVector WorldPos = GetActorTransform().TransformPosition(Pt.Position);
            DrawDebugPoint(World, WorldPos, Pt.Size, Pt.Color, false, 0.0f);
        }
    }
}

void ASofaTestActor::DrawDebugSurfaceActorSpace(const FSofaObjectState& ObjState)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    FSofaFrameSnapshot Snapshot;
    if (!SofaSubsystem->TryGetLatestSnapshot(Snapshot))
    {
        return;
    }
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
            const FVector WorldA = GetActorTransform().TransformPosition(Obj.DebugPoints[T.A].Position);
            const FVector WorldB = GetActorTransform().TransformPosition(Obj.DebugPoints[T.B].Position);
            const FVector WorldC = GetActorTransform().TransformPosition(Obj.DebugPoints[T.C].Position);
            DrawDebugLine(World, WorldA, WorldB, FColor::Cyan, false, 0.f, 0, 0.5f);
            DrawDebugLine(World, WorldB, WorldC, FColor::Cyan, false, 0.f, 0, 0.5f);
            DrawDebugLine(World, WorldC, WorldA, FColor::Cyan, false, 0.f, 0, 0.5f);
        }
    }
}