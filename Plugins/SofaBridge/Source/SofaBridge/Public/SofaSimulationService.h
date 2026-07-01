#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Containers/Queue.h"
#include "SofaSceneTypes.h"
#include "SofaSceneConfig.h"

class FSofaSimWorker;
class FSofaSceneBuilder;
struct FSofaRuntimeScene;

namespace sofa::core::objectmodel
{
    class BaseObject;
}

class SOFABRIDGE_API FSofaSimulationService
{
public:
    FSofaSimulationService();
    ~FSofaSimulationService();

    bool Initialize();
    void Shutdown();

    bool StartSimulation();
    void StopSimulation();

    void EnqueueCommand(const FSofaCommand& Command);
    bool TryGetLatestSnapshot(FSofaFrameSnapshot& OutSnapshot);

    ESofaSimState GetState() const { return State; }

    bool BuildPrototypeScene();
    bool StepSimulation(double DeltaTime);

    bool RegisterInteractorBinding(FName TargetId, sofa::core::objectmodel::BaseObject* TargetObject);
    bool SetInteractorTargetPose(FName TargetId, const FTransform& TargetPose);
    bool ClearInteractorTargetPose(FName TargetId);

    FSofaSceneConfig GetActiveSceneConfig() const { return ActiveSceneConfig; }

private:
    friend class FSofaSimWorker;
    friend class FSofaSceneBuilder;

    bool InitializeSofaRuntime();
    void PublishSnapshot(const FSofaFrameSnapshot& Snapshot);
    void ProcessPendingCommands();
    void HandleCommand(const FSofaCommand& Command);

    void ApplyInteractorTargetsToSimulation(const FSofaDeformableObjectConfig& Config);
    void ApplySingleInteractorTarget(sofa::core::objectmodel::BaseObject* TargetObject, const FSofaDeformableObjectConfig& Config, const FTransform& UEPose);

    void InitializeInteractorBindings();

private:
    TUniquePtr<FSofaRuntimeScene> SofaContext;

    TUniquePtr<FSofaSimWorker> Worker;

    mutable FCriticalSection CommandMutex;
    TQueue<FSofaCommand, EQueueMode::Mpsc> PendingCommands;

    mutable FCriticalSection SnapshotMutex;
    FSofaFrameSnapshot LatestSnapshot;

    ESofaSimState State = ESofaSimState::Stopped;
    uint64 FrameCounter = 0;
    double SimTime = 0.0;

    FSofaSceneConfig ActiveSceneConfig;

    FCriticalSection InteractorTargetsMutex;
    TMap<FName, FTransform> PendingInteractorTargetPoses;

    TMap<FName, sofa::core::objectmodel::BaseObject*> InteractorBindings;

    bool LoggedChildNodes = false;
};