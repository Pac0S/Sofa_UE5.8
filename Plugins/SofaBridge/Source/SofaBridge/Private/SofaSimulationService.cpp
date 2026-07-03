#include "SofaSimulationService.h"
#include "Logging/LogMacros.h"
#include "SofaIncludes.h"
#include "SofaSimulationWorker.h"
#include "SofaSceneBuilder.h"
#include "SofaRuntimeScene.h"
#include "SofaRuntimeTypes.h"
#include "SofaUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaService, Log, All);

#if SOFA_SDK_ENABLED
using FSofaGoalMechanicalObject = sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>;
#endif

struct FSofaToolBinding
{
    FName ToolId = NAME_None;
    FSofaRuntimeToolDescriptor Descriptor;

#if SOFA_SDK_ENABLED
    FSofaGoalMechanicalObject* GoalMechanicalObject = nullptr;
#endif

    bool IsValid() const
    {
#if SOFA_SDK_ENABLED
        return ToolId != NAME_None && GoalMechanicalObject != nullptr;
#else
        return false;
#endif
    }
};

struct FSofaToolBindingsStorage
{
    TMap<FName, FSofaToolBinding> Bindings;
};

namespace
{
#if SOFA_SDK_ENABLED

    using sofa::simulation::Node;

    static Node* FindChildNodeByName(Node* Parent, const char* ChildName)
    {
        if (!Parent || !ChildName)
        {
            return nullptr;
        }

        return Parent->getChild(ChildName);
    }

    static bool ApplyTargetToGoalMechanicalObject(
        sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>& GoalMO,
        const FSofaRuntimeToolDescriptor& ToolDesc,
        const FTransform& UETargetPose)
    {
        using FGoalMechanicalObject = sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>;
        using VecCoord = FGoalMechanicalObject::VecCoord;
        using Coord = FGoalMechanicalObject::Coord;

        const FVector SofaTarget = SofaCoordinateSystem::UnrealToolPoseToSofaPosition(UETargetPose, ToolDesc);

        auto* PositionData = GoalMO.findData("position");

        if (!PositionData)
        {
            return false;
        }

        auto* TypedPositionData = dynamic_cast<sofa::core::objectmodel::Data<VecCoord>*>(PositionData);

        if (!TypedPositionData)
        {
            return false;
        }

        sofa::helper::WriteAccessor<sofa::core::objectmodel::Data<VecCoord>> Positions(*TypedPositionData);

        if (Positions.size() == 0)
        {
            Positions.resize(1);
        }

        Positions[0] = Coord(static_cast<double>(SofaTarget.X), static_cast<double>(SofaTarget.Y), static_cast<double>(SofaTarget.Z));

        return true;
    }

    static bool ApplyTargetToGoalMechanicalObject(
        const FSofaToolBinding& Binding,
        const FTransform& UETargetPose)
    {
        if (!Binding.IsValid())
        {
            return false;
        }
        return ApplyTargetToGoalMechanicalObject(*Binding.GoalMechanicalObject, Binding.Descriptor, UETargetPose);
    }

    static FSofaGoalMechanicalObject* FindGoalMechanicalObject(
        sofa::simulation::Node* RootNode,
        const FName GoalNodeName,
        const FName GoalMechanicalObjectName)
    {
        if (!RootNode || GoalNodeName.IsNone() || GoalMechanicalObjectName.IsNone())
        {
            return nullptr;
        }

        sofa::simulation::Node* GoalNode = SofaSceneExtractor::FindChildOrDescendantNodeByName(RootNode, GoalNodeName.ToString());

        if (!GoalNode)
        {
            return nullptr;
        }

        sofa::core::objectmodel::BaseObject* BaseObj = GoalNode->getObject(TCHAR_TO_UTF8(*GoalMechanicalObjectName.ToString()));

        if (!BaseObj)
        {
            return nullptr;
        }

        return dynamic_cast<FSofaGoalMechanicalObject*>(BaseObj);
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

    InitializeToolBindings();

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

void FSofaSimulationService::PublishSnapshot(FSofaFrameSnapshot&& Snapshot)
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

    ApplyPendingToolInputsToSimulation();
    sofa::simulation::node::animate(SofaContext->RootNode.get(), DeltaTime);

    SimTime += DeltaTime;
    ++FrameCounter;

    FSofaFrameSnapshot Snapshot;
    Snapshot.FrameId = FrameCounter;
    Snapshot.SimTime = SimTime;
    Snapshot.State = State;
    Snapshot.Objects.Reserve(SofaContext->RuntimeObjects.Num());
    Snapshot.Tools.Reserve(SofaContext->RuntimeTools.Num());

    for (const FSofaRuntimeObjectDescriptor& RuntimeObj : SofaContext->RuntimeObjects)
    {
        FSofaObjectState ObjState;
        ObjState.ObjectId = FName(*RuntimeObj.ObjectNodeName);
        ObjState.WorldTransform = RuntimeObj.UnrealAnchorTransform;

        FString ExtractError;
        if (!SofaSceneExtractor::ExtractRenderableSurfaceMesh(*SofaContext, RuntimeObj, ObjState, ExtractError))
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ExtractRenderableSurfaceMesh failed for runtime object '%s': %s"),
                *RuntimeObj.ObjectNodeName,
                *ExtractError);
        }
        Snapshot.Objects.Add(MoveTemp(ObjState));
    }

    for (const FSofaRuntimeToolDescriptor& ToolDesc : SofaContext->RuntimeTools)
    {
        FSofaToolState ToolState;
        ToolState.ToolId = ToolDesc.ToolNodeName;
        ToolState.WorldTransform = FTransform::Identity;
        ToolState.bValid = false;

        FSofaGoalMechanicalObject* GoalMO =
            FindGoalMechanicalObject(
                SofaContext->RootNode.get(),
                ToolDesc.ToolNodeName,
                ToolDesc.ControlMechanicalObjectName);

        if (!GoalMO)
        {
            UE_LOG(LogSofaService, Verbose,
                TEXT("StepSimulation: no GoalMO found for tool '%s'."),
                *ToolDesc.ToolNodeName.ToString());

            Snapshot.Tools.Add(MoveTemp(ToolState));
            continue;
        }

        const auto Positions = GoalMO->readPositions();
        if (Positions.size() == 0)
        {
            UE_LOG(LogSofaService, Verbose,
                TEXT("StepSimulation: GoalMO '%s/%s' has no positions."),
                *ToolDesc.ToolNodeName.ToString(),
                *ToolDesc.ControlMechanicalObjectName.ToString());

            Snapshot.Tools.Add(MoveTemp(ToolState));
            continue;
        }

        const auto& P = Positions[0];
        const FVector SofaPos(static_cast<float>(P[0]), static_cast<float>(P[1]), static_cast<float>(P[2]));
        const FTransform SofaPose(FQuat::Identity, SofaPos);
        const FVector UnrealPos = SofaCoordinateSystem::SofaToolPoseToUnrealPosition(SofaPose, ToolDesc);

        FTransform ToolWorld = FTransform::Identity;
        ToolWorld.SetLocation(UnrealPos);
        ToolState.WorldTransform = ToolWorld;
        ToolState.bValid = true;

        Snapshot.Tools.Add(MoveTemp(ToolState));
    }

    PublishSnapshot(MoveTemp(Snapshot));
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
        if (FName(*RuntimeObj.ObjectNodeName) == ObjectId)
        {
            OutMaterialPath = RuntimeObj.VisualMaterialPath;
            return !OutMaterialPath.IsEmpty();
        }
    }

    return false;
}

bool FSofaSimulationService::SubmitToolInput(const FSofaToolInputState& Input)
{
    if (Input.ToolId.IsNone())
    {
        return false;
    }

    FScopeLock Lock(&PendingToolInputsMutex);
    PendingToolInputs.FindOrAdd(Input.ToolId) = Input;
    return true;
}

void FSofaSimulationService::ConsumePendingToolInputs(TArray<FSofaToolInputState>& OutInputs)
{
    OutInputs.Reset();

    FScopeLock Lock(&PendingToolInputsMutex);

    OutInputs.Reserve(PendingToolInputs.Num());
    for (const TPair<FName, FSofaToolInputState>& Pair : PendingToolInputs)
    {
        OutInputs.Add(Pair.Value);
    }

    PendingToolInputs.Reset();
}

void FSofaSimulationService::InitializeToolBindings()
{
    ResetToolBindings();

#if !SOFA_SDK_ENABLED
    return;
#else
    if (!ToolBindingsStorage.IsValid())
    {
        UE_LOG(LogSofaService, Warning, TEXT("InitializeToolBindings: ToolBindingsStorage is invalid."));
        return;
    }

    if (!SofaContext)
    {
        UE_LOG(LogSofaService, Warning, TEXT("InitializeToolBindings: SofaContext is null."));
        return;
    }

    if (!SofaContext->RootNode)
    {
        UE_LOG(LogSofaService, Warning, TEXT("InitializeToolBindings: RootNode is null."));
        return;
    }

    for (const FSofaRuntimeToolDescriptor& ToolDesc : SofaContext->RuntimeTools)
    {
        if (ToolDesc.ToolNodeName.IsNone())
        {
            UE_LOG(LogSofaService, Warning, TEXT("InitializeToolBindings: skipped tool with empty ToolId."));
            continue;
        }

        FSofaGoalMechanicalObject* GoalMO = FindGoalMechanicalObject(SofaContext->RootNode.get(), ToolDesc.ToolNodeName, ToolDesc.ControlMechanicalObjectName);

        if (!GoalMO)
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("InitializeToolBindings: failed to bind tool '%s' (GoalMO='%s')."),
                *ToolDesc.ToolNodeName.ToString(),
                *ToolDesc.ControlMechanicalObjectName.ToString());
            continue;
        }

        FSofaToolBinding Binding;
        Binding.ToolId = ToolDesc.ToolNodeName;
        Binding.Descriptor = ToolDesc;
        Binding.GoalMechanicalObject = GoalMO;

        ToolBindingsStorage->Bindings.Add(Binding.ToolId, MoveTemp(Binding));

        UE_LOG(LogSofaService, Log,
            TEXT("InitializeToolBindings: registered tool '%s' -> object='%s'."),
            *ToolDesc.ToolNodeName.ToString(),
            *ToolDesc.ControlMechanicalObjectName.ToString());
    }

    UE_LOG(LogSofaService, Log,
        TEXT("InitializeToolBindings: %d tool binding(s) registered."),
        ToolBindingsStorage->Bindings.Num());
#endif
}

void FSofaSimulationService::ResetToolBindings()
{
    if (!ToolBindingsStorage.IsValid())
    {
        ToolBindingsStorage = MakeShared<FSofaToolBindingsStorage>();
        return;
    }
    ToolBindingsStorage->Bindings.Reset();
}

void FSofaSimulationService::ApplyPendingToolInputsToSimulation()
{
#if !SOFA_SDK_ENABLED
    return;
#else
    if (!ToolBindingsStorage.IsValid())
    {
        return;
    }

    TArray<FSofaToolInputState> Inputs;
    ConsumePendingToolInputs(Inputs);

    if (Inputs.IsEmpty())
    {
        return;
    }

    for (const FSofaToolInputState& Input : Inputs)
    {
        if (Input.ToolId.IsNone())
        {
            UE_LOG(LogSofaService, Verbose,
                TEXT("ApplyPendingToolInputsToSimulation: skipped input with empty ToolId."));
            continue;
        }

        if (!Input.bEnabled)
        {
            UE_LOG(LogSofaService, Verbose,
                TEXT("ApplyPendingToolInputsToSimulation: tool '%s' disabled, skipping."),
                *Input.ToolId.ToString());
            continue;
        }

        FSofaToolBinding* Binding = ToolBindingsStorage->Bindings.Find(Input.ToolId);
        if (!Binding)
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ApplyPendingToolInputsToSimulation: no binding found for tool '%s'."),
                *Input.ToolId.ToString());
            continue;
        }

        if (!Binding->IsValid())
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ApplyPendingToolInputsToSimulation: invalid binding for tool '%s'."),
                *Input.ToolId.ToString());
            continue;
        }

        if (!ApplyTargetToGoalMechanicalObject(
            *Binding->GoalMechanicalObject,
            Binding->Descriptor,
            Input.TargetPose))
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ApplyPendingToolInputsToSimulation: failed to apply pose for tool '%s'."),
                *Input.ToolId.ToString());
            continue;
        }

        UE_LOG(LogSofaService, Verbose,
            TEXT("ApplyPendingToolInputsToSimulation: applied pose for tool '%s' at t=%.6f."),
            *Input.ToolId.ToString(),
            Input.Timestamp);
    }
#endif
}