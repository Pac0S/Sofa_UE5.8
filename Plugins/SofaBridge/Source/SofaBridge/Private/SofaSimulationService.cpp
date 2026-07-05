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

    static Node* FindChildNodeByName(Node* Parent, const char* ChildName)
    {
        if (!Parent || !ChildName)
        {
            return nullptr;
        }

        return Parent->getChild(ChildName);
    }

    static const FSofaResolvedBinding* FindBinding(
        const FSofaRuntimeScene& Scene,
        FName OwnerId,
        ESofaBindingUsage Usage)
    {
        for (const FSofaResolvedBinding& Binding : Scene.Bindings)
        {
            if (Binding.Generation != Scene.SceneGeneration)
            {
                continue;
            }

            if (Binding.Descriptor.OwnerId == OwnerId &&
                Binding.Descriptor.Usage == Usage)
            {
                return &Binding;
            }
        }

        return nullptr;
    }

    static FSofaResolvedBinding* FindBinding(
        FSofaRuntimeScene& Scene,
        FName OwnerId,
        ESofaBindingUsage Usage)
    {
        for (FSofaResolvedBinding& Binding : Scene.Bindings)
        {
            if (Binding.Generation != Scene.SceneGeneration)
            {
                continue;
            }

            if (Binding.Descriptor.OwnerId == OwnerId &&
                Binding.Descriptor.Usage == Usage)
            {
                return &Binding;
            }
        }

        return nullptr;
    }

    static const FSofaIndexedNode* FindIndexedNodeByPath(
        const FSofaRuntimeScene& Scene,
        const FString& NodePath)
    {
        if (NodePath.IsEmpty())
        {
            return nullptr;
        }

        const FSofaIndexedNode* IndexedNode = Scene.NodeIndexByPath.Find(NodePath);
        if (!IndexedNode)
        {
            return nullptr;
        }

        if (IndexedNode->Generation != Scene.SceneGeneration)
        {
            return nullptr;
        }

        return IndexedNode;
    }

    static const FSofaIndexedObject* FindIndexedObjectByKey(
        const FSofaRuntimeScene& Scene,
        const FString& ObjectKey)
    {
        if (ObjectKey.IsEmpty())
        {
            return nullptr;
        }

        const FSofaIndexedObject* IndexedObject = Scene.ObjectIndexByKey.Find(ObjectKey);
        if (!IndexedObject)
        {
            return nullptr;
        }

        if (IndexedObject->Generation != Scene.SceneGeneration)
        {
            return nullptr;
        }

        return IndexedObject;
    }

    static sofa::core::objectmodel::BaseObject* ResolveObjectOnNode(
        sofa::simulation::Node* Node,
        const FName ObjectName)
    {
        if (!Node || ObjectName.IsNone())
        {
            return nullptr;
        }

        return Node->getObject(TCHAR_TO_UTF8(*ObjectName.ToString()));
    }

    static sofa::simulation::Node* ResolveNodeByPathSegments(
        const FSofaRuntimeScene& Scene,
        const FString& NodePath)
    {
        if (!Scene.RootNode || NodePath.IsEmpty())
        {
            return nullptr;
        }

        if (NodePath == TEXT("root"))
        {
            return Scene.RootNode.get();
        }

        TArray<FString> Segments;
        NodePath.ParseIntoArray(Segments, TEXT("/"), true);

        if (Segments.Num() == 0 || Segments[0] != TEXT("root"))
        {
            return nullptr;
        }

        sofa::simulation::Node* Current = Scene.RootNode.get();

        for (int32 i = 1; i < Segments.Num(); ++i)
        {
            if (!Current)
            {
                return nullptr;
            }

            Current = Current->getChild(TCHAR_TO_UTF8(*Segments[i]));
        }

        return Current;
    }

    static sofa::core::objectmodel::BaseObject* ResolveObjectByKey(
        const FSofaRuntimeScene& Scene,
        const FString& ObjectKey)
    {
        const FSofaIndexedObject* IndexedObject = FindIndexedObjectByKey(Scene, ObjectKey);
        if (!IndexedObject)
        {
            return nullptr;
        }
        if (!FindIndexedNodeByPath(Scene, IndexedObject->NodePath))
        {
            return nullptr;
        }

        sofa::simulation::Node* Node = ResolveNodeByPathSegments(Scene, IndexedObject->NodePath);
        if (!Node)
        {
            return nullptr;
        }

        return Node->getObject(TCHAR_TO_UTF8(*IndexedObject->ObjectName));
    }

    static bool ApplyTargetToBinding(
        const FSofaRuntimeScene& Scene,
        const FSofaResolvedBinding& Binding,
        const FSofaRuntimeToolDescriptor& ToolDesc,
        const FTransform& UETargetPose)
    {
        if (!Binding.IsValidForGeneration(Scene.SceneGeneration))
        {
            return false;
        }

        sofa::core::objectmodel::BaseObject* Object = ResolveObjectByKey(Scene, Binding.Descriptor.ObjectKey);
        if (!Object)
        {
            return false;
        }

        const FVector SofaTarget =
            SofaCoordinateSystem::UnrealToolPoseToSofaPosition(UETargetPose, ToolDesc);

        sofa::core::objectmodel::BaseData* PositionData = Object->findData("position");
        if (!PositionData)
        {
            return false;
        }

        using RigidCoord = sofa::defaulttype::Rigid3Types::Coord;
        using RigidVecCoord = sofa::type::vector<RigidCoord>;
        using RigidData = sofa::core::objectmodel::Data<RigidVecCoord>;

        if (auto* TypedData = dynamic_cast<RigidData*>(PositionData))
        {
            RigidVecCoord& Positions = *TypedData->beginEdit();

            if (Positions.empty())
            {
                TypedData->endEdit();
                return false;
            }

            RigidCoord& Pose = Positions[0];
            Pose.getCenter() = sofa::type::Vec3(
                static_cast<double>(SofaTarget.X),
                static_cast<double>(SofaTarget.Y),
                static_cast<double>(SofaTarget.Z));
            Pose.getOrientation() = sofa::type::Quat<double>(0.0, 0.0, 0.0, 1.0);

            TypedData->endEdit();
            return true;
        }

        using Vec3Coord = sofa::defaulttype::Vec3Types::Coord;
        using Vec3VecCoord = sofa::type::vector<Vec3Coord>;
        using Vec3Data = sofa::core::objectmodel::Data<Vec3VecCoord>;

        if (auto* TypedData = dynamic_cast<Vec3Data*>(PositionData))
        {
            Vec3VecCoord& Positions = *TypedData->beginEdit();

            if (Positions.empty())
            {
                TypedData->endEdit();
                return false;
            }

            Positions[0] = Vec3Coord(
                static_cast<double>(SofaTarget.X),
                static_cast<double>(SofaTarget.Y),
                static_cast<double>(SofaTarget.Z));

            TypedData->endEdit();
            return true;
        }

        return false;
    }

    static bool ReadToolPoseFromBinding(
        const FSofaRuntimeScene& Scene,
        const FSofaResolvedBinding& Binding,
        FVector& OutSofaPosition)
    {
        OutSofaPosition = FVector::ZeroVector;

        if (!Binding.IsValidForGeneration(Scene.SceneGeneration) || !Scene.RootNode)
        {
            return false;
        }

        sofa::core::objectmodel::BaseObject* Object = ResolveObjectByKey(Scene, Binding.Descriptor.ObjectKey);
        if (!Object)
        {
            return false;
        }

        using RigidMechanicalObject = sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Rigid3Types>;

        if (const auto* MO = dynamic_cast<const RigidMechanicalObject*>(Object))
        {
            const auto& Positions = MO->readPositions();
            if (Positions.size() == 0)
            {
                return false;
            }

            const auto& C = Positions[0].getCenter();
            OutSofaPosition = FVector(
                static_cast<float>(C[0]),
                static_cast<float>(C[1]),
                static_cast<float>(C[2]));
            return true;
        }

        using Vec3MechanicalObject = sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>;

        if (const auto* MO = dynamic_cast<const Vec3MechanicalObject*>(Object))
        {
            const auto& Positions = MO->readPositions();
            if (Positions.size() == 0)
            {
                return false;
            }

            const auto& P = Positions[0];
            OutSofaPosition = FVector(
                static_cast<float>(P[0]),
                static_cast<float>(P[1]),
                static_cast<float>(P[2]));
            return true;
        }

        return false;
    }

#endif
}

static const FSofaRuntimeToolDescriptor* FindRuntimeToolDescriptor(
    const FSofaRuntimeScene& Scene,
    FName ToolId)
{
    for (const FSofaRuntimeToolDescriptor& ToolDesc : Scene.RuntimeTools)
    {
        if (ToolDesc.ToolNodeName == ToolId)
        {
            return &ToolDesc;
        }
    }
    return nullptr;
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
    StopSimulation();
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

    const FSofaSceneBuilder::FBuildResult BuildResult = FSofaSceneBuilder::BuildPrototypeScene(*SofaContext, Request);

    if (!BuildResult.bSuccess)
    {
        State = ESofaSimState::Error;
        UE_LOG(LogSofaService, Error, TEXT("Scene build failed: %s"), *BuildResult.ErrorMessage);
        return false;
    }

    FScopeLock SceneLock(&SceneMutex);
    InitializeBindings_NoLock();

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

    UE_LOG(LogSofaService, Warning,
        TEXT("[STOP] Before stop | WorkerValid=%s Root=%p Sim=%p State=%d"),
        Worker.IsValid() ? TEXT("true") : TEXT("false"),
        SofaContext ? SofaContext->RootNode.get() : nullptr,
        SofaContext ? SofaContext->SimulationPtr.get() : nullptr,
        (int32)State);

    State = ESofaSimState::Stopping;

    if (Worker.IsValid())
    {
        Worker->RequestStop();
        Worker->Wait();
    }
    FScopeLock SceneLock(&SceneMutex);
    ResetBindings_NoLock();

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

    FScopeLock SceneLock(&SceneMutex);

    if (State != ESofaSimState::Running || !SofaContext || !SofaContext->RootNode || !SofaContext->SimulationPtr)
    {
        return false;
    }

    ApplyPendingToolInputsToSimulation_NoLock();
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

        const FName ObjectId = FName(*RuntimeObj.ObjectNodeName);

        FString ExtractError;

        const FSofaResolvedBinding* ObjectBinding = FindBinding(*SofaContext, ObjectId, ESofaBindingUsage::ObjectMechanical);

        if (!ObjectBinding)
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("StepSimulation: no ObjectMechanical binding for runtime object '%s'."),
                *RuntimeObj.ObjectNodeName);
        }
        else if (!SofaSceneExtractor::ExtractRenderableSurfaceMesh(
            *SofaContext,
            RuntimeObj,
            *ObjectBinding,
            ObjState,
            ExtractError))
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
        ToolState.UnrealLocalToolTransform = FTransform::Identity;
        ToolState.bValid = false;

        const FSofaResolvedBinding* Binding = FindBinding(*SofaContext, ToolDesc.ToolNodeName, ESofaBindingUsage::ToolControl);

        if (!Binding || !Binding->IsValidForGeneration(SofaContext->SceneGeneration))
        {
            UE_LOG(LogSofaService, Verbose,
                TEXT("StepSimulation: no valid binding for tool '%s'."),
                *ToolDesc.ToolNodeName.ToString());

            Snapshot.Tools.Add(MoveTemp(ToolState));
            continue;
        }

        FVector SofaPos = FVector::ZeroVector;
        if (!ReadToolPoseFromBinding(*SofaContext, *Binding, SofaPos))
        {
            UE_LOG(LogSofaService, Verbose,
                TEXT("StepSimulation: binding '%s' has no readable position."),
                *ToolDesc.ToolNodeName.ToString());

            Snapshot.Tools.Add(MoveTemp(ToolState));
            continue;
        }

        const FTransform SofaPose(FQuat::Identity, SofaPos);
        const FVector UnrealPos =
            SofaCoordinateSystem::SofaToolPoseToUnrealPosition(SofaPose, ToolDesc);

        FTransform UnrealLocalToolTransform = FTransform::Identity;
        UnrealLocalToolTransform.SetLocation(UnrealPos);
        ToolState.UnrealLocalToolTransform = UnrealLocalToolTransform;
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

void FSofaSimulationService::InitializeBindings_NoLock()
{
    ResetBindings_NoLock();

#if !SOFA_SDK_ENABLED
    return;
#else
    if (!SofaContext)
    {
        UE_LOG(LogSofaService, Warning, TEXT("InitializeBindings: SofaContext is null."));
        return;
    }

    if (!SofaContext->RootNode)
    {
        UE_LOG(LogSofaService, Warning, TEXT("InitializeBindings: RootNode is null."));
        return;
    }

    UE_LOG(LogSofaService, Warning,
        TEXT("[INIT BIND] Begin | Root=%p RuntimeTools=%d RuntimeObjects=%d Bindings=%d"),
        SofaContext->RootNode.get(),
        SofaContext->RuntimeTools.Num(),
        SofaContext->RuntimeObjects.Num(),
        SofaContext->Bindings.Num());

    SofaContext->Bindings.Reserve(SofaContext->RuntimeTools.Num() + SofaContext->RuntimeObjects.Num());

    for (const FSofaRuntimeToolDescriptor& ToolDesc : SofaContext->RuntimeTools)
    {
        if (ToolDesc.ToolNodeName.IsNone() ||
            ToolDesc.ControlMechanicalObjectName.IsNone())
        {
            continue;
        }

        FSofaResolvedBinding& Binding = SofaContext->Bindings.AddDefaulted_GetRef();

        const FString ToolNodePath = FString::Printf(TEXT("root/%s"), *ToolDesc.ToolNodeName.ToString());
        //const FString ControlNodePath = FString::Printf(TEXT("%s/%s"), *ToolNodePath, *ToolDesc.ControlNodeName.ToString());
        const FString ControlObjectKey = FString::Printf( TEXT("%s::%s"), *ToolNodePath, *ToolDesc.ControlMechanicalObjectName.ToString());

        if (!FindIndexedNodeByPath(*SofaContext, ToolNodePath) || !FindIndexedObjectByKey(*SofaContext, ControlObjectKey))
        {
            continue;
        }

        Binding.Descriptor.BindingId = FName(*FString::Printf(TEXT("%s:Control"), *ToolDesc.ToolNodeName.ToString()));
        Binding.Descriptor.OwnerId = ToolDesc.ToolNodeName;
        Binding.Descriptor.Usage = ESofaBindingUsage::ToolControl;
        Binding.Descriptor.NodePath = ToolNodePath;
        Binding.Descriptor.ObjectKey = ControlObjectKey;
        Binding.Generation = SofaContext->SceneGeneration;
        Binding.bResolved = true;
    }

    for (const FSofaRuntimeObjectDescriptor& RuntimeObj : SofaContext->RuntimeObjects)
    {
        const FName OwnerId(*RuntimeObj.ObjectNodeName);
        if (OwnerId.IsNone() || RuntimeObj.MechanicalObjectName.IsEmpty())
        {
            continue;
        }

        FSofaResolvedBinding& Binding = SofaContext->Bindings.AddDefaulted_GetRef();

        const FString ObjectNodePath = FString::Printf(TEXT("root/%s"), *RuntimeObj.ObjectNodeName);
        const FString MechanicalObjectKey = FString::Printf(TEXT("%s::%s"), *ObjectNodePath, *RuntimeObj.MechanicalObjectName);
        if (!RuntimeObj.SurfaceNodeName.IsEmpty() && !RuntimeObj.SurfaceTopologyName.IsEmpty())
        {
            const FString SurfaceNodePath = FString::Printf(TEXT("%s/%s"), *ObjectNodePath, *RuntimeObj.SurfaceNodeName);
            const FString SurfaceTopologyObjectKey = FString::Printf(TEXT("%s::%s"), *SurfaceNodePath, *RuntimeObj.SurfaceTopologyName);

            if (FindIndexedNodeByPath(*SofaContext, SurfaceNodePath) &&
                FindIndexedObjectByKey(*SofaContext, SurfaceTopologyObjectKey))
            {
                Binding.Descriptor.SurfaceTopologyObjectKey = SurfaceTopologyObjectKey;
            }
        }

        if (!RuntimeObj.VisualNodeName.IsEmpty())
        {
            const FString VisualNodePath =
                FString::Printf(TEXT("%s/%s"), *ObjectNodePath, *RuntimeObj.VisualNodeName);

            if (FindIndexedNodeByPath(*SofaContext, VisualNodePath))
            {
                Binding.Descriptor.VisualNodePath = VisualNodePath;

                if (!RuntimeObj.VisualMechanicalObjectName.IsEmpty())
                {
                    const FString VisualObjectKey = FString::Printf(TEXT("%s::%s"), *VisualNodePath, *RuntimeObj.VisualMechanicalObjectName);

                    if (FindIndexedObjectByKey(*SofaContext, VisualObjectKey))
                    {
                        Binding.Descriptor.VisualObjectKey = VisualObjectKey;
                    }
                }

                if (!RuntimeObj.VisualTopologyName.IsEmpty())
                {
                    const FString VisualTopologyObjectKey = FString::Printf(TEXT("%s::%s"), *VisualNodePath, *RuntimeObj.VisualTopologyName);
                    if (FindIndexedObjectByKey(*SofaContext, VisualTopologyObjectKey))
                    {
                        Binding.Descriptor.VisualTopologyObjectKey = VisualTopologyObjectKey;
                    }
                }
            }
        }

        if (!FindIndexedNodeByPath(*SofaContext, ObjectNodePath) || !FindIndexedObjectByKey(*SofaContext, MechanicalObjectKey))
        {
            continue;
        }

        Binding.Descriptor.BindingId = FName(*FString::Printf(TEXT("%s:Mechanical"), *RuntimeObj.ObjectNodeName));
        Binding.Descriptor.OwnerId = FName(RuntimeObj.ObjectNodeName);
        Binding.Descriptor.Usage = ESofaBindingUsage::ObjectMechanical;
        Binding.Descriptor.NodePath = ObjectNodePath;
        Binding.Descriptor.ObjectKey = MechanicalObjectKey;
        Binding.Generation = SofaContext->SceneGeneration;
        Binding.bResolved = true;
    }

    UE_LOG(LogSofaService, Log,
        TEXT("InitializeBindings: %d binding(s) registered."),
        SofaContext->Bindings.Num());
#endif
}

void FSofaSimulationService::InitializeBindings()
{
    FScopeLock SceneLock(&SceneMutex);
    InitializeBindings_NoLock();
}

void FSofaSimulationService::ResetBindings_NoLock()
{
    UE_LOG(LogSofaService, Warning,
        TEXT("[RESET BIND] Before | SofaContext=%p Root=%p Bindings.Num=%d"),
        SofaContext.Get(),
        SofaContext ? SofaContext->RootNode.get() : nullptr,
        SofaContext ? SofaContext->Bindings.Num() : -1);

    if (!SofaContext)
    {
        UE_LOG(LogSofaService, Error, TEXT("ResetBindings_NoLock: invalid SofaContext"));
        return;
    }

    SofaContext->Bindings.Reset();
}

void FSofaSimulationService::ResetBindings()
{
    FScopeLock SceneLock(&SceneMutex);
    ResetBindings_NoLock();
}

void FSofaSimulationService::ApplyPendingToolInputsToSimulation_NoLock()
{
#if !SOFA_SDK_ENABLED
    return;
#else
    if (!SofaContext)
    {
        UE_LOG(LogSofaService, Error, TEXT("ApplyPendingToolInputsToSimulation: invalid SofaContext"));
        return;
    }

    if (SofaContext->Bindings.IsEmpty())
    {
        UE_LOG(LogSofaService, Warning, TEXT("ApplyPendingToolInputsToSimulation: no binding found"));
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
        if (Input.ToolId.IsNone() || !Input.bEnabled)
        {
            continue;
        }

        FSofaResolvedBinding* Binding = FindBinding(*SofaContext, Input.ToolId, ESofaBindingUsage::ToolControl);

        if (!Binding || !Binding->IsValidForGeneration(SofaContext->SceneGeneration))
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ApplyPendingToolInputsToSimulation: no valid binding for tool '%s'."),
                *Input.ToolId.ToString());
            continue;
        }

        const FSofaRuntimeToolDescriptor* ToolDesc = FindRuntimeToolDescriptor(*SofaContext, Input.ToolId);

        if (!ToolDesc)
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ApplyPendingToolInputsToSimulation: missing runtime descriptor for tool '%s'."),
                *Input.ToolId.ToString());
            continue;
        }

        if (!ApplyTargetToBinding(*SofaContext, *Binding, *ToolDesc, Input.TargetPose))
        {
            UE_LOG(LogSofaService, Warning,
                TEXT("ApplyPendingToolInputsToSimulation: failed to apply pose for tool '%s'."),
                *Input.ToolId.ToString());
            continue;
        }
    }
#endif
}

void FSofaSimulationService::ApplyPendingToolInputsToSimulation()
{
    FScopeLock SceneLock(&SceneMutex);
    ApplyPendingToolInputsToSimulation_NoLock();
}