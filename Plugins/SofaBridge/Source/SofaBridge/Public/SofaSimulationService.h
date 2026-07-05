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
struct FSofaToolBindingsStorage;

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

    bool SubmitToolInput(const FSofaToolInputState& Input);

private:
    friend class FSofaSimWorker;
    friend class FSofaSceneBuilder;

    bool InitializeSofaRuntime();
    void PublishSnapshot(FSofaFrameSnapshot&& Snapshot);
    void ProcessPendingCommands();
    void HandleCommand(const FSofaCommand& Command);
    void InitializeBindings();
    void InitializeBindings_NoLock();
    void ResetBindings();
    void ResetBindings_NoLock();
    void ApplyPendingToolInputsToSimulation();
    void ApplyPendingToolInputsToSimulation_NoLock();
    void ConsumePendingToolInputs(TArray<FSofaToolInputState>& OutInputs);

private:

    TUniquePtr<FSofaSimWorker> Worker;

    mutable FCriticalSection SceneMutex;
    TUniquePtr<FSofaRuntimeScene> SofaContext;

    mutable FCriticalSection CommandMutex;
    TQueue<FSofaCommand, EQueueMode::Mpsc> PendingCommands;

    mutable FCriticalSection SnapshotMutex;
    FSofaFrameSnapshot LatestSnapshot;

    FCriticalSection PendingToolInputsMutex;
    TMap<FName, FSofaToolInputState> PendingToolInputs;

    TArray<FSofaRuntimeToolDescriptor> RuntimeTools;

    TSharedPtr<struct FSofaToolBindingsStorage> ToolBindingsStorage;

    ESofaSimState State = ESofaSimState::Stopped;
    uint64 FrameCounter = 0;
    double SimTime = 0.0;

    bool LoggedChildNodes = false;
};