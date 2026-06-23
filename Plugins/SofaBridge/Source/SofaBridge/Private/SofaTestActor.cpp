// SofaTestActor.cpp
#include "SofaTestActor.h"
#include "Engine/World.h"
#include "SofaSceneSubsystem.h"

void ASofaTestActor::BeginPlay()
{
    Super::BeginPlay();

    if (auto* Sub = GetWorld()->GetSubsystem<USofaSceneSubsystem>())
    {
        Sub->StartPrototypeSimulation();
        UE_LOG(LogTemp, Log, TEXT("SofaTestActor: simulation started"));
    }
}

void ASofaTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (auto* Sub = GetWorld()->GetSubsystem<USofaSceneSubsystem>())
    {
        Sub->StopPrototypeSimulation();
    }
    Super::EndPlay(EndPlayReason);
}