#include "SofaSimulationWorker.h"
#include "HAL/PlatformProcess.h"
#include "SofaSimulationService.h"

FSofaSimWorker::FSofaSimWorker(FSofaSimulationService* InOwner)
    : Owner(InOwner)
{
}

FSofaSimWorker::~FSofaSimWorker()
{
    RequestStop();

    if (Thread)
    {
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }
}

bool FSofaSimWorker::Start()
{
    if (Thread)
    {
        return false;
    }

    bStopRequested = false;
    Thread = FRunnableThread::Create(this, TEXT("SofaSimWorker"));
    return Thread != nullptr;
}

void FSofaSimWorker::RequestStop()
{
    bStopRequested = true;
}

bool FSofaSimWorker::Init()
{
    return Owner != nullptr;
}

uint32 FSofaSimWorker::Run()
{
    constexpr double FixedDt = 1.0 / 100.0;

    while (!bStopRequested)
    {
        if (Owner)
        {
            Owner->ProcessPendingCommands();

            if (Owner->GetState() == ESofaSimState::Running)
            {
                Owner->StepSimulation(FixedDt);
            }
        }

        FPlatformProcess::SleepNoStats(static_cast<float>(FixedDt));
    }

    return 0;
}

void FSofaSimWorker::Stop()
{
    bStopRequested = true;
}

void FSofaSimWorker::Exit()
{
}