#include "SofaSceneSubsystem.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

USofaSceneSubsystem::~USofaSceneSubsystem() = default;

bool USofaSceneSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;

    return World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Game;
}

void USofaSceneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Service = MakeUnique<FSofaSimulationService>();
    Service->Initialize();
}

void USofaSceneSubsystem::Deinitialize()
{
    if (Service)
    {
        Service->Shutdown();
        Service.Reset();
    }

    Super::Deinitialize();
}

void USofaSceneSubsystem::Tick(float DeltaTime)
{
    if (!Service)
    {
        return;
    }

    FSofaFrameSnapshot Snapshot;
    if (!Service->TryGetLatestSnapshot(Snapshot))
    {
        return;
    }
    /*for (const FSofaObjectState& Obj : Snapshot.Objects)
    {
        UE_LOG(LogTemp, Warning, TEXT("Tick draw '%s': %d points at loc %s"),
            *Obj.ObjectId.ToString(), Obj.DebugPoints.Num(), *Obj.WorldTransform.GetLocation().ToString());
    }*/

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (const FSofaObjectState& Obj : Snapshot.Objects)
    {
        DrawDebugCoordinateSystem(
            World,
            Obj.WorldTransform.GetLocation(),
            Obj.WorldTransform.Rotator(),
            20.0f,
            false,
            0.0f,
            0,
            1.0f);
    }

    

    UE_LOG(LogTemp, Verbose, TEXT("SOFA Frame %llu, SimTime=%.4f, Objects=%d"),
        Snapshot.FrameId, Snapshot.SimTime, Snapshot.Objects.Num());
}

TStatId USofaSceneSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(USofaSceneSubsystem, STATGROUP_Tickables);
}

void USofaSceneSubsystem::ConfigurePrototypeScene(const FSofaPrototypeSceneRequest& Request)
{
    PendingPrototypeSceneRequest = Request;
    bHasPrototypeSceneRequest = true;

    UE_LOG(LogTemp, Log, TEXT("SOFA prototype scene configured. UseFilePath=%s SceneFilePath=%s SceneName=%s"),
        Request.bUseSceneFilePath ? TEXT("true") : TEXT("false"),
        *Request.SceneFilePath,
        *Request.SceneName);
}

void USofaSceneSubsystem::StartSimulation()
{
    if (Service)
    {
        Service->StartSimulation();
    }
}

bool USofaSceneSubsystem::StartPrototypeSimulation()
{
    if (!bHasPrototypeSceneRequest)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start SOFA prototype simulation: no prototype scene request configured."));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start SOFA prototype simulation: invalid world."));
        return false;
    }


    bool bStarted = false;
    if (Service)
    {
        bStarted = Service->StartPrototypeSimulation(PendingPrototypeSceneRequest);
    }
    if (!bStarted)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start SOFA prototype simulation."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("SOFA prototype simulation started successfully."));
    return true;
}

void USofaSceneSubsystem::StopPrototypeSimulation()
{
    if (Service)
    {
        Service->StopSimulation();
    }
}

void USofaSceneSubsystem::PauseSimulation()
{
    if (!Service)
    {
        return;
    }

    FSofaCommand Cmd;
    Cmd.Type = ESofaCommandType::Pause;
    Service->EnqueueCommand(Cmd);
}

void USofaSceneSubsystem::ResumeSimulation()
{
    if (!Service)
    {
        return;
    }

    FSofaCommand Cmd;
    Cmd.Type = ESofaCommandType::Resume;
    Service->EnqueueCommand(Cmd);
}

void USofaSceneSubsystem::ClearPrototypeSceneRequest()
{
    PendingPrototypeSceneRequest = FSofaPrototypeSceneRequest();
    bHasPrototypeSceneRequest = false;
}

bool USofaSceneSubsystem::HasPrototypeSceneRequest() const
{
    return bHasPrototypeSceneRequest;
}

FSofaPrototypeSceneRequest USofaSceneSubsystem::GetPrototypeSceneRequest() const
{
    return PendingPrototypeSceneRequest;
}

bool USofaSceneSubsystem::TryGetLatestSnapshot(FSofaFrameSnapshot& OutSnapshot) const
{
    if (!Service)
    {
        return false;
    }

    return Service->TryGetLatestSnapshot(OutSnapshot);
}

bool USofaSceneSubsystem::SetInteractorTargetPose(FName TargetId, const FTransform& TargetPose)
{
    if (!Service)
    {
        return false;
    }
    return Service->SetInteractorTargetPose(TargetId, TargetPose);
}

bool USofaSceneSubsystem::ClearInteractorTargetPose(FName TargetId)
{
    if (!Service)
    {
        return false;
    }
    return Service->ClearInteractorTargetPose(TargetId);
}

bool USofaSceneSubsystem::GetObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const
{
    if (!Service)
    {
        OutMaterialPath.Reset();
        return false;
    }
    return Service->GetRuntimeObjectMaterialPath(ObjectId, OutMaterialPath);
}