#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Containers/Queue.h"
#include "SofaRuntimeTypes.h"
#include "SofaCommandTypes.h"

class FSofaSimWorker;
class FSofaSceneBuilder;
struct FSofaRuntimeScene;
struct FSofaRuntimeObjectDescriptor;

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

    bool StartPrototypeSimulation(const FSofaPrototypeSceneRequest& Request);
    void StopSimulation();
    bool StepSimulation(double DeltaTime);

    void EnqueueCommand(const FSofaCommand& Command);
    bool TryGetLatestSnapshot(FSofaFrameSnapshot& OutSnapshot);

    ESofaSimState GetState() const { return State; }

    bool GetRuntimeObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const;

private:
    friend class FSofaSimWorker;
    friend class FSofaSceneBuilder;

    bool InitializeSofaRuntime();
    void PublishSnapshot(const FSofaFrameSnapshot& Snapshot);
    void ProcessPendingCommands();
    void HandleCommand(const FSofaCommand& Command);

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

    FCriticalSection InteractorTargetsMutex;
    TMap<FName, FTransform> PendingInteractorTargetPoses;

    bool LoggedChildNodes = false;
};