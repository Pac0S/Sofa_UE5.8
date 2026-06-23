#include "SofaSimulationService.h"
#include "Logging/LogMacros.h"
#include "SofaIncludes.h"
#include "SofaSimulationWorker.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaService, Log, All);

struct FSofaSimulationService::FSofaContext
{
    sofa::simulation::Simulation::SPtr SimulationPtr;
    sofa::simulation::NodeSPtr RootNode;
};

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
    SofaContext = MakeUnique<FSofaContext>();

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

bool FSofaSimulationService::StartSimulation()
{
    if (!Worker.IsValid())
    {
        return false;
    }

    if (!BuildPrototypeScene())
    {
        State = ESofaSimState::Error;
        return false;
    }

    State = ESofaSimState::Running;
    return Worker->Start();
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
        BuildPrototypeScene();
        break;

    case ESofaCommandType::Stop:
        State = ESofaSimState::Stopping;
        break;

    default:
        break;
    }
}

bool FSofaSimulationService::BuildPrototypeScene()
{
#if !SOFA_SDK_ENABLED
    return false;
#else
    using sofa::simulation::Simulation;
    using sofa::simulation::NodeSPtr;

    UE_LOG(LogSofaService, Log, TEXT("Building prototype SOFA scene"));

    Simulation::SPtr Simu = sofa::simpleapi::createSimulation("DAG");
    if (!Simu)
    {
        UE_LOG(LogSofaService, Error, TEXT("Failed to create SOFA simulation"));
        return false;
    }

    NodeSPtr Root = sofa::simpleapi::createRootNode(Simu, "root");
    if (!Root)
    {
        UE_LOG(LogSofaService, Error, TEXT("Failed to create SOFA root node"));
        return false;
    }

    Root->setName("UE5Root");
    Root->setAnimate(true);
    Root->setDt(0.01);
    Root->setGravity(sofa::type::Vec3(0.0, -9.81, 0.0));

    if (!SofaContext) return false;
    SofaContext->SimulationPtr = Simu;
    SofaContext->RootNode = Root;

    FrameCounter = 0;
    SimTime = 0.0;

    UE_LOG(LogSofaService, Log, TEXT("Prototype SOFA scene built"));
    return true;
#endif
}

bool FSofaSimulationService::StepSimulation(double DeltaTime)
{
#if !SOFA_SDK_ENABLED
    return false;
#else
    if (State != ESofaSimState::Running) return false;
    if (!SofaContext) return false;
    if (!SofaContext->RootNode) return false;
    if (!SofaContext->RootNode) return false;
    sofa::simulation::node::animate(SofaContext->RootNode.get(), DeltaTime);

    SimTime += DeltaTime;
    ++FrameCounter;

    FSofaFrameSnapshot Snapshot;
    Snapshot.FrameId = FrameCounter;
    Snapshot.SimTime = SimTime;
    Snapshot.State = State;

    FSofaObjectState RootState;
    RootState.ObjectId = FName(TEXT("Root"));
    Snapshot.Objects.Add(RootState);

    PublishSnapshot(Snapshot);
    return true;
#endif
}