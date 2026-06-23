#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"

class FSofaSimulationService;

class FSofaSimWorker : public FRunnable
{
public:
    explicit FSofaSimWorker(FSofaSimulationService* InOwner);
    virtual ~FSofaSimWorker();

    bool Start();
    void RequestStop();

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

private:
    FSofaSimulationService* Owner = nullptr;
    FRunnableThread* Thread = nullptr;
    FThreadSafeBool bStopRequested = false;
};