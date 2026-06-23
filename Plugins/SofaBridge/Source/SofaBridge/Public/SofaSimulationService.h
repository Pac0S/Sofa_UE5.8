#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Containers/Queue.h"
#include "SofaSceneTypes.h"

class FSofaSimWorker;

namespace sofa::simulation
{
    class Node;
    class Simulation;
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

private:
    friend class FSofaSimWorker;

    bool InitializeSofaRuntime();
    void PublishSnapshot(const FSofaFrameSnapshot& Snapshot);
    void ProcessPendingCommands();
    void HandleCommand(const FSofaCommand& Command);

private:
    struct FSofaContext;
    TUniquePtr<FSofaContext> SofaContext;

    TUniquePtr<FSofaSimWorker> Worker;

    mutable FCriticalSection CommandMutex;
    TQueue<FSofaCommand, EQueueMode::Mpsc> PendingCommands;

    mutable FCriticalSection SnapshotMutex;
    FSofaFrameSnapshot LatestSnapshot;

    ESofaSimState State = ESofaSimState::Stopped;
    uint64 FrameCounter = 0;
    double SimTime = 0.0;
};