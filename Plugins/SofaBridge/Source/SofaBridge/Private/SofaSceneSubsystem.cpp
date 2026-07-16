#include "SofaSceneSubsystem.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

#include "SofaCommandTypes.h"

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

bool USofaSceneSubsystem::GetObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const
{
    if (!Service)
    {
        OutMaterialPath.Reset();
        return false;
    }
    return Service->GetRuntimeObjectMaterialPath(ObjectId, OutMaterialPath);
}

bool USofaSceneSubsystem::FindRuntimeToolDescriptor(FSofaRuntimeToolDescriptor& ToolDesc, FName ToolId) const
{
    if (!Service)
    {
        return false;
    }
    return Service->FindRuntimeToolDescriptor(ToolDesc, ToolId);
}

bool USofaSceneSubsystem::FindRuntimeObjectDescriptor(FSofaRuntimeObjectDescriptor& ObjectDesc, FName ObjectId) const
{
    if (!Service)
    {
        return false;
    }
    return Service->FindRuntimeObjectDescriptor(ObjectDesc, ObjectId);
}

bool USofaSceneSubsystem::SubmitToolInput(const FSofaToolInputState& Input)
{
    if (!Service)
    {
        return false;
    }

    return Service->SubmitToolInput(Input);
}

bool USofaSceneSubsystem::GetStaticCollisionDebugPointsByMesh(
    TMap<FName, TArray<FSofaDebugPoint>>& OutPointsByMesh,
    FString& OutError)
{
    OutPointsByMesh.Reset();
    OutError.Reset();

    if (!Service)
    {
        OutError = TEXT("SimulationService is null.");
        UE_LOG(LogTemp, Warning, TEXT("[SOFA][Subsystem] %s"), *OutError);
        return false;
    }

    const bool bSuccess =
        Service->GetStaticCollisionDebugPointsByMesh(OutPointsByMesh, OutError);

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SOFA][Subsystem] GetStaticCollisionDebugPointsByMesh failed: %s"), *OutError);
    }

    return bSuccess;
}