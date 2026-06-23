#include "SofaSceneSubsystem.h"
#include "Engine/World.h"

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
    if (Service->TryGetLatestSnapshot(Snapshot))
    {
        UE_LOG(LogTemp, Verbose, TEXT("SOFA Frame %llu, SimTime=%.4f"),
            Snapshot.FrameId, Snapshot.SimTime);
    }
}

TStatId USofaSceneSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(USofaSceneSubsystem, STATGROUP_Tickables);
}

void USofaSceneSubsystem::StartPrototypeSimulation()
{
    if (Service)
    {
        Service->StartSimulation();
    }
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