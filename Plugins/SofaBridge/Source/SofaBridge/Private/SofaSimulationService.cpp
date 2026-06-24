#include "SofaSimulationService.h"
#include "Logging/LogMacros.h"
#include "SofaIncludes.h"
#include "SofaSimulationWorker.h"
#include "SofaSceneBuilder.h"
#include "SofaRuntimeScene.h"
#include "SofaUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaService, Log, All);

namespace
{
#if SOFA_SDK_ENABLED

    using sofa::simulation::Node;
    using sofa::component::statecontainer::MechanicalObject;
    using sofa::defaulttype::Vec3Types;

    struct FSofaInteractorBinding
    {
        FName TargetId = NAME_None;
        sofa::core::objectmodel::BaseObject* TargetObject = nullptr;
        sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>* GoalMechanicalObject = nullptr;

        bool bIsValid() const
        {
            return TargetObject != nullptr || GoalMechanicalObject != nullptr;
        }
    };

    static void ApplyTargetToGoalMechanicalObject(
        sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>& GoalMO,
        const FSofaDeformableObjectConfig& Config,
        const FTransform& UEPose)
    {
        using Coord = sofa::defaulttype::Vec3Types::Coord;
        FVector SofaPos = SofaCoordinateSystem::UnrealToSofaPosition(UEPose.GetLocation(), Config);
        const sofa::type::Vec3d SofaVecPos = sofa::type::Vec3d(SofaPos.X, SofaPos.Y, SofaPos.Z);

        auto WriteAccessor = GoalMO.writePositions();
        auto& Positions = *WriteAccessor;

        if (Positions.empty())
        {
            Positions.resize(1);
        }

        Positions[0] = Coord(SofaVecPos[0], SofaVecPos[1], SofaVecPos[2]);
    }

    static Node* FindChildNodeByName(Node* Parent, const char* ChildName)
    {
        if (!Parent || !ChildName)
        {
            return nullptr;
        }

        return Parent->getChild(ChildName);
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
    if (!SofaContext)
    {
        return false;
    }

    UE_LOG(LogSofaService, Log, TEXT("Building prototype SOFA scene"));

    FSofaSceneConfig Config;
    Config.SceneId = TEXT("PrototypeScene");
    Config.TimeStep = 0.01;
    Config.Gravity = FVector(0.0, -9.81, 0.0);

    FSofaDeformableObjectConfig& DeformableObjectConfig = Config.Objects.AddDefaulted_GetRef();
    DeformableObjectConfig.Id = TEXT("Liver01");
    DeformableObjectConfig.VolumeMeshPath = TEXT("C:/Users/Pakito/Documents/Projets/AniSim/sofa/src/share/mesh/liver.msh");
    
    DeformableObjectConfig.YoungModulus = 50.0;
    DeformableObjectConfig.PoissonRatio = 0.3;
    DeformableObjectConfig.TotalMass = 1.0;
    DeformableObjectConfig.Scale = 10.0;
    DeformableObjectConfig.Translation = FVector::ZeroVector;
    DeformableObjectConfig.Rotation = FRotator::ZeroRotator;
    DeformableObjectConfig.bFixedBase = false;

    FSofaVisualObjectConfig& VisualObjectConfig = DeformableObjectConfig.VisualConfig;
    VisualObjectConfig.VisualMeshPath = TEXT("C:/Users/Pakito/Documents/Projets/AniSim/sofa/src/share/mesh/liver.obj");
    VisualObjectConfig.MaterialPath = TEXT("C:/Users/Pakito/Documents/Projets/AniSim/sofa/src/share/mesh/liver.mtl");

    VisualObjectConfig.bUseVisualMesh = true;
    VisualObjectConfig.bUseTexture = false;
    VisualObjectConfig.bHandleSeams = true;

    ActiveSceneConfig = Config;

    const FSofaSceneBuilder::FBuildResult BuildResult =
        FSofaSceneBuilder::BuildPrototypeScene(*SofaContext, Config);

    InitializeInteractorBindings();

    if (!BuildResult.bSuccess)
    {
        UE_LOG(LogSofaService, Error, TEXT("Scene build failed: %s"), *BuildResult.ErrorMessage);
        return false;
    }

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
    if (!SofaContext->SimulationPtr) return false;

    for (const FSofaDeformableObjectConfig& ObjConfig : ActiveSceneConfig.Objects)
    {
        ApplyInteractorTargetsToSimulation(ObjConfig);
    }
    sofa::simulation::node::animate(SofaContext->RootNode.get(), DeltaTime);

    SimTime += DeltaTime;
    ++FrameCounter;

    FSofaFrameSnapshot Snapshot;
    Snapshot.FrameId = FrameCounter;
    Snapshot.SimTime = SimTime;
    Snapshot.State = State;

    for (const FSofaDeformableObjectConfig& ObjConfig : ActiveSceneConfig.Objects)
    {
        FSofaObjectState ObjState;
        ObjState.ObjectId = FName(*ObjConfig.Id);
        ObjState.WorldTransform = FTransform(ObjConfig.Rotation, ObjConfig.Translation);

        FString ExtractError;
        if (!SofaSceneExtractor::ExtractRenderableSurfaceMesh(*SofaContext, ObjConfig, ObjState, ExtractError))
        {
            UE_LOG(LogSofaService, Warning, TEXT("ExtractRenderableSurfaceMesh failed for '%s': %s"),
                *ObjConfig.Id, *ExtractError);
        }
        Snapshot.Objects.Add(MoveTemp(ObjState));
    }

    PublishSnapshot(Snapshot);
    return true;
#endif
}

bool FSofaSimulationService::RegisterInteractorBinding(
    FName TargetId,
    sofa::core::objectmodel::BaseObject* TargetObject)
{
    if (!TargetObject)
    {
        return false;
    }

    InteractorBindings.Add(TargetId, TargetObject);
    return true;
}

bool FSofaSimulationService::SetInteractorTargetPose(FName TargetId, const FTransform& TargetPose)
{
    FScopeLock Lock(&InteractorTargetsMutex);
    PendingInteractorTargetPoses.Add(TargetId, TargetPose);
    return true;
}

bool FSofaSimulationService::ClearInteractorTargetPose(FName TargetId)
{
    FScopeLock Lock(&InteractorTargetsMutex);
    PendingInteractorTargetPoses.Remove(TargetId);
    return true;
}

void FSofaSimulationService::ApplyInteractorTargetsToSimulation(const FSofaDeformableObjectConfig& Config)
{
    TMap<FName, FTransform> LocalTargets;
    {
        FScopeLock Lock(&InteractorTargetsMutex);
        LocalTargets = PendingInteractorTargetPoses;
    }

    if (LocalTargets.IsEmpty())
    {
        return;
    }

    for (const TPair<FName, FTransform>& It : LocalTargets)
    {
        sofa::core::objectmodel::BaseObject* const *FoundTargetObject = InteractorBindings.Find(It.Key);
        if (!FoundTargetObject || !(*FoundTargetObject))
        {
            continue;
        }
        ApplySingleInteractorTarget(*FoundTargetObject, Config, It.Value);
    }
}

void FSofaSimulationService::ApplySingleInteractorTarget(
    sofa::core::objectmodel::BaseObject* TargetObject,
    const FSofaDeformableObjectConfig& Config,
    const FTransform& UEPose)
{
    if (!TargetObject)
    {
        return;
    }

    auto* GoalMO = dynamic_cast<sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>*>(TargetObject);

    if (!GoalMO)
    {
        return;
    }

    ApplyTargetToGoalMechanicalObject(*GoalMO, Config, UEPose);
}

void FSofaSimulationService::InitializeInteractorBindings()
{
#if !SOFA_SDK_ENABLED
    return;
#else
    InteractorBindings.Empty();

    if (!SofaContext || !SofaContext->RootNode)
    {
        UE_LOG(LogSofaService, Warning, TEXT("InitializeInteractorBindings: invalid SOFA context or root node"));
        return;
    }

    for (const FSofaDeformableObjectConfig& ObjConfig : ActiveSceneConfig.Objects)
    {
        if (ObjConfig.Id.IsEmpty())
        {
            continue;
        }

        sofa::simulation::Node* ObjectNode =
            SofaContext->RootNode->getChild(TCHAR_TO_UTF8(*ObjConfig.Id));

        if (!ObjectNode)
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("InitializeInteractorBindings: child node '%s' not found"),
                *ObjConfig.Id);
            continue;
        }

        if (sofa::core::objectmodel::BaseObject* ForceObject =
            ObjectNode->getObject(std::string("InteractorForce")))
        {
            const FName BindingId(*FString::Printf(TEXT("%s.InteractorForce"), *ObjConfig.Id));
            RegisterInteractorBinding(BindingId, ForceObject);

            UE_LOG(LogSofaService, Log,
                TEXT("Registered binding '%s' -> %s"),
                *BindingId.ToString(),
                UTF8_TO_TCHAR(ForceObject->getName().c_str()));
        }

        if (sofa::core::objectmodel::BaseObject* MStateObject =
            ObjectNode->getObject(std::string("mstate")))
        {
            const FName BindingId(*FString::Printf(TEXT("%s.mstate"), *ObjConfig.Id));
            RegisterInteractorBinding(BindingId, MStateObject);

            UE_LOG(LogSofaService, Log,
                TEXT("Registered binding '%s' -> %s"),
                *BindingId.ToString(),
                UTF8_TO_TCHAR(MStateObject->getName().c_str()));
        }
    }
#endif
}