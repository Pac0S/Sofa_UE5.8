#include "SofaSimulationService.h"
#include "Logging/LogMacros.h"
#include "SofaIncludes.h"
#include "SofaSimulationWorker.h"
#include "SofaSceneBuilder.h"
#include "SofaRuntimeScene.h"
#include "SofaRuntimeTypes.h"
#include "SofaUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaService, Log, All);

namespace
{
#if SOFA_SDK_ENABLED

    using sofa::simulation::Node;
    using sofa::component::statecontainer::MechanicalObject;
    using sofa::defaulttype::Vec3Types;

    static Node* FindChildNodeByName(Node* Parent, const char* ChildName)
    {
        if (!Parent || !ChildName)
        {
            return nullptr;
        }

        return Parent->getChild(ChildName);
    }

    const FSofaRuntimeObjectDescriptor* FindRuntimeObjectForInteractor(
        const FSofaRuntimeScene& SofaContext,
        FName InteractorId)
    {
        const FString InteractorIdString = InteractorId.ToString();

        FString ParentNodeName;
        FString ComponentName;
        if (!InteractorIdString.Split(TEXT("."), &ParentNodeName, &ComponentName))
        {
            return nullptr;
        }

        for (const FSofaRuntimeObjectDescriptor& RuntimeObj : SofaContext.RuntimeObjects)
        {
            if (RuntimeObj.SimulationNodeName == ParentNodeName ||
                RuntimeObj.ObjectId == ParentNodeName)
            {
                return &RuntimeObj;
            }
        }

        return nullptr;
    }
#endif
}


FSofaSimulationService::FSofaSimulationService()
{
}

FSofaSimulationService::~FSofaSimulationService()
{
    Shutdown();
}

bool FSofaSimulationService::Initialize()
{
    if (!InitializeSofaRuntime())
    {
        State = ESofaSimState::Error;
        return false;
    }

    LatestSnapshot.State = ESofaSimState::Stopped;
    Worker = MakeUnique<FSofaSimWorker>(this);
    SofaContext = MakeUnique<FSofaRuntimeScene>();

    State = ESofaSimState::Stopped;
    UE_LOG(LogSofaService, Log, TEXT("FSofaSimulationService initialized"));
    return true;
}

void FSofaSimulationService::Shutdown()
{
    StopSimulation();
    Worker.Reset();
    SofaContext.Reset();

    State = ESofaSimState::Stopped;
    UE_LOG(LogSofaService, Log, TEXT("FSofaSimulationService shutdown"));
}

bool FSofaSimulationService::InitializeSofaRuntime()
{
#if !SOFA_SDK_ENABLED
    UE_LOG(LogSofaService, Error, TEXT("SOFA SDK disabled"));
    return false;
#else
    sofa::simulation::common::init();
    sofa::simulation::graph::init();
    UE_LOG(LogSofaService, Log, TEXT("SOFA runtime initialized"));
    return true;
#endif
}

bool FSofaSimulationService::StartPrototypeSimulation(const FSofaPrototypeSceneRequest& Request)
{
#if !SOFA_SDK_ENABLED
    return false;
#else
    if (!SofaContext)
    {
        State = ESofaSimState::Error;
        UE_LOG(LogSofaService, Error, TEXT("BuildPrototypeScene failed: SofaContext is null."));
        return false;
    }

    if (!Worker.IsValid())
    {
        State = ESofaSimState::Error;
        UE_LOG(LogSofaService, Error, TEXT("BuildPrototypeScene failed: Worker is not valid."));
        return false;
    }

    if (Request.SceneFilePath.IsEmpty() && Request.SceneName.IsEmpty())
    {
        State = ESofaSimState::Error;
        UE_LOG(LogSofaService, Error, TEXT("BuildPrototypeScene failed: no SceneFilePath or SceneName provided."));
        return false;
    }

    UE_LOG(LogSofaService, Log,
        TEXT("Building prototype SOFA scene. UseFilePath=%s SceneFilePath=%s SceneName=%s"),
        Request.bUseSceneFilePath ? TEXT("true") : TEXT("false"),
        *Request.SceneFilePath,
        *Request.SceneName);

    const FSofaSceneBuilder::FBuildResult BuildResult =
        FSofaSceneBuilder::BuildPrototypeScene(*SofaContext, Request);

    if (!BuildResult.bSuccess)
    {
        State = ESofaSimState::Error;
        UE_LOG(LogSofaService, Error, TEXT("Scene build failed: %s"), *BuildResult.ErrorMessage);
        return false;
    }

    FrameCounter = 0;
    SimTime = 0.0;

    UE_LOG(LogSofaService, Log,
        TEXT("Prototype SOFA scene built successfully from '%s'"),
        Request.bUseSceneFilePath ? *Request.SceneFilePath : *Request.SceneName);

    State = ESofaSimState::Running;
    return Worker->Start();
#endif
}


void FSofaSimulationService::StopSimulation()
{
    State = ESofaSimState::Stopping;

    if (Worker.IsValid())
    {
        Worker->RequestStop();
    }

    State = ESofaSimState::Stopped;
}

void FSofaSimulationService::EnqueueCommand(const FSofaCommand& Command)
{
    PendingCommands.Enqueue(Command);
}

bool FSofaSimulationService::TryGetLatestSnapshot(FSofaFrameSnapshot& OutSnapshot)
{
    FScopeLock Lock(&SnapshotMutex);
    OutSnapshot = LatestSnapshot;
    return true;
}

void FSofaSimulationService::PublishSnapshot(const FSofaFrameSnapshot& Snapshot)
{
    FScopeLock Lock(&SnapshotMutex);
    LatestSnapshot = Snapshot;
}

void FSofaSimulationService::ProcessPendingCommands()
{
    FSofaCommand Command;
    while (PendingCommands.Dequeue(Command))
    {
        HandleCommand(Command);
    }
}

void FSofaSimulationService::HandleCommand(const FSofaCommand& Command)
{
    switch (Command.Type)
    {
    case ESofaCommandType::Pause:
        State = ESofaSimState::Paused;
        break;

    case ESofaCommandType::Resume:
        State = ESofaSimState::Running;
        break;

    case ESofaCommandType::ResetScene:
        //BuildPrototypeScene();
        break;

    case ESofaCommandType::Stop:
        State = ESofaSimState::Stopping;
        break;

    default:
        break;
    }
}

bool FSofaSimulationService::StepSimulation(double DeltaTime)
{
#if !SOFA_SDK_ENABLED
    return false;
#else
    if (State != ESofaSimState::Running)
    {
        return false;
    }

    if (!SofaContext)
    {
        return false;
    }

    if (!SofaContext->RootNode)
    {
        return false;
    }

    if (!SofaContext->SimulationPtr)
    {
        return false;
    }

    sofa::simulation::node::animate(SofaContext->RootNode.get(), DeltaTime);

    SimTime += DeltaTime;
    ++FrameCounter;

    FSofaFrameSnapshot Snapshot;
    Snapshot.FrameId = FrameCounter;
    Snapshot.SimTime = SimTime;
    Snapshot.State = State;
    Snapshot.Objects.Reserve(SofaContext->RuntimeObjects.Num());

    for (const FSofaRuntimeObjectDescriptor& RuntimeObj : SofaContext->RuntimeObjects)
    {
        FSofaObjectState ObjState;
        ObjState.ObjectId = FName(*RuntimeObj.ObjectId);
        ObjState.WorldTransform = RuntimeObj.UnrealObjectTransform;

        FString ExtractError;
        if (!SofaSceneExtractor::ExtractRenderableSurfaceMesh(*SofaContext, RuntimeObj, ObjState, ExtractError))
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ExtractRenderableSurfaceMesh failed for runtime object '%s': %s"),
                *RuntimeObj.ObjectId,
                *ExtractError);
        }
        Snapshot.Objects.Add(MoveTemp(ObjState));
    }

    PublishSnapshot(Snapshot);
    return true;
#endif
}

bool FSofaSimulationService::GetRuntimeObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const
{
    OutMaterialPath.Reset();

    if (!SofaContext)
    {
        return false;
    }

    for (const FSofaRuntimeObjectDescriptor& RuntimeObj : SofaContext->RuntimeObjects)
    {
        if (FName(*RuntimeObj.ObjectId) == ObjectId)
        {
            OutMaterialPath = RuntimeObj.VisualMaterialPath;
            return !OutMaterialPath.IsEmpty();
        }
    }

    return false;
}