#include "SofaToolProxyActor.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"

#include "SofaSceneSubsystem.h"
#include "SofaRuntimeTypes.h"

ASofaToolProxyActor::ASofaToolProxyActor()
{
    PrimaryActorTick.bCanEverTick = true;

    RootSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RootSphere"));
    RootComponent = RootSphere;
    RootSphere->InitSphereRadius(8.0f);
    RootSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootSphere->SetHiddenInGame(true);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetGenerateOverlapEvents(false);

    InputMoveSpeed = 150.0f;
    bDriveFromKeyboard = true;
    bFollowSnapshot = true;
    bDrawDebug = true;
    DebugSphereRadius = 6.0f;
    DebugAxisLength = 20.0f;
    DebugTextZOffset = 12.0f;

    ToolId = TEXT("PrimaryTool");
}

void ASofaToolProxyActor::BeginPlay()
{
    Super::BeginPlay();
}

void ASofaToolProxyActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bInitialized || !SofaSubsystem)
    {
        return;
    }

    if (bDriveFromKeyboard)
    {
        UpdateKeyboardControl(DeltaSeconds);
        SubmitCurrentToolInput();
    }

    if (bFollowSnapshot)
    {
        RefreshFromSnapshot();
    }

    if (bDrawDebug)
    {
        DrawToolDebug();
    }
}

void ASofaToolProxyActor::InitializeToolProxy(
    USofaSceneSubsystem* InSubsystem,
    FName InToolId)
{
    SofaSubsystem = InSubsystem;
    ToolId = InToolId;
    bInitialized = (SofaSubsystem != nullptr && !ToolId.IsNone());

    UE_LOG(LogTemp, Log,
        TEXT("SofaToolProxyActor initialized. ToolId=%s, bInitialized=%s"),
        *ToolId.ToString(),
        bInitialized ? TEXT("true") : TEXT("false"));
}

void ASofaToolProxyActor::UpdateKeyboardControl(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    FVector MoveDir = FVector::ZeroVector;

    if (PC->IsInputKeyDown(EKeys::Y))
    {
        MoveDir.X += 1.0f;
    }
    if (PC->IsInputKeyDown(EKeys::H))
    {
        MoveDir.X -= 1.0f;
    }
    if (PC->IsInputKeyDown(EKeys::J))
    {
        MoveDir.Y += 1.0f;
    }
    if (PC->IsInputKeyDown(EKeys::G))
    {
        MoveDir.Y -= 1.0f;
    }
    if (PC->IsInputKeyDown(EKeys::U))
    {
        MoveDir.Z += 1.0f;
    }
    if (PC->IsInputKeyDown(EKeys::T))
    {
        MoveDir.Z -= 1.0f;
    }

    if (!MoveDir.IsNearlyZero())
    {
        MoveDir = MoveDir.GetSafeNormal();

        const FVector OldLocation = GetActorLocation();
        const FVector NewLocation = OldLocation + MoveDir * InputMoveSpeed * DeltaSeconds;

        SetActorLocation(NewLocation);
    }
}

void ASofaToolProxyActor::SubmitCurrentToolInput()
{
    if (!SofaSubsystem || ToolId.IsNone())
    {
        return;
    }

    FSofaToolInputState Input;
    Input.ToolId = ToolId;
    Input.TargetPose = GetActorTransform();
    Input.bEnabled = true;
    Input.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    const bool bOk = SofaSubsystem->SubmitToolInput(Input);

    if (!bOk)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("SubmitCurrentToolInput failed for tool '%s'."),
            *ToolId.ToString());
    }
}

void ASofaToolProxyActor::RefreshFromSnapshot()
{
    if (!SofaSubsystem || ToolId.IsNone())
    {
        return;
    }

    FSofaFrameSnapshot Snapshot;
    if (!SofaSubsystem->TryGetLatestSnapshot(Snapshot))
    {
        return;
    }

    for (const FSofaToolState& ToolState : Snapshot.Tools)
    {
        if (ToolState.ToolId != ToolId)
        {
            continue;
        }

        if (!ToolState.bValid)
        {
            return;
        }

        SetActorTransform(ToolState.WorldTransform);
        LastSnapshotTransform = ToolState.WorldTransform;
        bHasSnapshotPose = true;
        return;
    }
}

void ASofaToolProxyActor::DrawToolDebug()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector Pos = GetActorLocation();
    const FTransform Xf = GetActorTransform();

    DrawDebugSphere(
        World,
        Pos,
        DebugSphereRadius,
        12,
        FColor::Green,
        false,
        0.0f,
        0,
        1.5f);

    DrawDebugLine(
        World,
        Pos,
        Pos + Xf.GetUnitAxis(EAxis::X) * DebugAxisLength,
        FColor::Red,
        false,
        0.0f,
        0,
        1.5f);

    DrawDebugLine(
        World,
        Pos,
        Pos + Xf.GetUnitAxis(EAxis::Y) * DebugAxisLength,
        FColor::Green,
        false,
        0.0f,
        0,
        1.5f);

    DrawDebugLine(
        World,
        Pos,
        Pos + Xf.GetUnitAxis(EAxis::Z) * DebugAxisLength,
        FColor::Blue,
        false,
        0.0f,
        0,
        1.5f);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const FString DebugLabel = FString::Printf(
        TEXT("Tool: %s\nLoc: X=%.1f Y=%.1f Z=%.1f\nSnapshot: %s"),
        *ToolId.ToString(),
        Pos.X, Pos.Y, Pos.Z,
        bHasSnapshotPose ? TEXT("yes") : TEXT("no"));

    DrawDebugString(
        World,
        Pos + FVector(0.0f, 0.0f, DebugTextZOffset),
        DebugLabel,
        nullptr,
        FColor::White,
        0.0f,
        false);
#endif
}