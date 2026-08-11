#include "SofaToolActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "SofaSceneSubsystem.h"

ASofaToolActor::ASofaToolActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    RootSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RootSphere"));
    RootSphere->SetupAttachment(SceneRoot);
    RootSphere->InitSphereRadius(8.0f);
    RootSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootSphere->SetHiddenInGame(true);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(SceneRoot);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetGenerateOverlapEvents(false);

    ToolId = TEXT("TOOL_Primary");
    DesiredLocalTransform = FTransform::Identity;
    CurrentSimulatedLocalTransform = FTransform::Identity;
    LastSubmittedLocalTransform = FTransform::Identity;
}

void ASofaToolActor::BeginPlay()
{
    Super::BeginPlay();

    /*if (bInitialized)
    {
        SubmitCurrentToolInput();
    }*/
    //bDriveFromKeyboard = false;
}

void ASofaToolActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bInitialized || !SofaSubsystem)
    {
        return;
    }

    
    if (bDriveFromKeyboard && bHasReceivedInitialSnapshot)
    {
        UpdateKeyboardControl(DeltaSeconds);
        SubmitCurrentToolInput();
    }

    if (bDrawDebug)
    {
        DrawToolDebug();
    }
}

void ASofaToolActor::InitializeToolActor(USofaSceneSubsystem* InSofaSubsystem, FName InToolId)
{
    SofaSubsystem = InSofaSubsystem;
    ToolId = InToolId;
    bInitialized = (SofaSubsystem != nullptr && !ToolId.IsNone());
}

void ASofaToolActor::SetDesiredLocalTransform(const FTransform& InLocalTransform)
{
    DesiredLocalTransform = InLocalTransform;
}

void ASofaToolActor::SetStaticMesh(UStaticMesh* InStaticMesh)
{
    if (VisualMesh)
    {
        VisualMesh->SetStaticMesh(InStaticMesh);
    }
}

void ASofaToolActor::SubmitCurrentToolInput()
{
    if (!SofaSubsystem || ToolId.IsNone())
    {
        return;
    }

    FSofaToolInputState Input;
    Input.ToolId = ToolId;
    Input.TargetPose = DesiredLocalTransform;
    Input.bEnabled = true;
    Input.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    if (SofaSubsystem->SubmitToolInput(Input))
    {
        LastSubmittedLocalTransform = DesiredLocalTransform;
    }
}

void ASofaToolActor::RefreshFromSnapshot(const FSofaFrameSnapshot& Snapshot)
{
    if (!bFollowSnapshot)
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
            UE_LOG(LogTemp, Log, TEXT("Tool is not valid"));
            return;
        }

        CurrentSimulatedLocalTransform = ToolState.UnrealLocalToolTransform;
        if (!bHasReceivedInitialSnapshot)
        {
            SetDesiredLocalTransform(CurrentSimulatedLocalTransform);
            LastSubmittedLocalTransform = DesiredLocalTransform;
            bHasReceivedInitialSnapshot = true;
        }

        SetActorRelativeTransform(CurrentSimulatedLocalTransform);
        bHasSnapshotPose = true;

        return;
    }
}

void ASofaToolActor::UpdateKeyboardControl(float DeltaSeconds)
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

    if (PC->IsInputKeyDown(EKeys::Y)) { MoveDir.X += 1.0f; }
    if (PC->IsInputKeyDown(EKeys::H)) { MoveDir.X -= 1.0f; }
    if (PC->IsInputKeyDown(EKeys::J)) { MoveDir.Y += 1.0f; }
    if (PC->IsInputKeyDown(EKeys::G)) { MoveDir.Y -= 1.0f; }
    if (PC->IsInputKeyDown(EKeys::U)) { MoveDir.Z += 1.0f; }
    if (PC->IsInputKeyDown(EKeys::T)) { MoveDir.Z -= 1.0f; }

    if (MoveDir.IsNearlyZero())
    {
        return;
    }

    MoveDir = MoveDir.GetSafeNormal();
    const FVector NewLocalLocation = CurrentSimulatedLocalTransform.GetLocation() + MoveDir * InputMoveSpeed * DeltaSeconds;
    const FTransform NewLocalTransform(CurrentSimulatedLocalTransform.GetRotation(), NewLocalLocation, CurrentSimulatedLocalTransform.GetScale3D());
    SetDesiredLocalTransform(NewLocalTransform);
}

void ASofaToolActor::DrawToolDebug() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FTransform WorldToolTransform = ToolLocalToWorldTransform(CurrentSimulatedLocalTransform);
    const FVector Pos = WorldToolTransform.GetLocation();
    const FTransform Xf = WorldToolTransform;

    DrawDebugSphere(World, Pos, DebugSphereRadius, 12, FColor::Green, false, 0.0f, 0, 1.5f);
    DrawDebugLine(World, Pos, Pos + Xf.GetUnitAxis(EAxis::X) * DebugAxisLength, FColor::Red, false, 0.0f, 0, 1.5f);
    DrawDebugLine(World, Pos, Pos + Xf.GetUnitAxis(EAxis::Y) * DebugAxisLength, FColor::Green, false, 0.0f, 0, 1.5f);
    DrawDebugLine(World, Pos, Pos + Xf.GetUnitAxis(EAxis::Z) * DebugAxisLength, FColor::Blue, false, 0.0f, 0, 1.5f);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const FString DebugLabel = FString::Printf(
        TEXT("Tool: %s\nDesiredLocal: X=%.1f Y=%.1f Z=%.1f\nSnapshot: %s"),
        *ToolId.ToString(),
        DesiredLocalTransform.GetLocation().X,
        DesiredLocalTransform.GetLocation().Y,
        DesiredLocalTransform.GetLocation().Z,
        bHasSnapshotPose ? TEXT("yes") : TEXT("no"));

    DrawDebugString(World, Pos + FVector(0.0f, 0.0f, DebugTextZOffset), DebugLabel, nullptr, FColor::White, 0.0f, false);
#endif
}

FTransform ASofaToolActor::ToolWorldToLocalTransform(const FTransform& WorldTransform) const
{
    const AActor* SceneActor = GetAttachParentActor();
    if (!SceneActor)
    {
        return WorldTransform;
    }

    return WorldTransform.GetRelativeTransform(SceneActor->GetActorTransform());
}

FTransform ASofaToolActor::ToolLocalToWorldTransform(const FTransform& LocalTransform) const
{
    const AActor* SceneActor = GetAttachParentActor();
    if (!SceneActor)
    {
        return LocalTransform;
    }

    return LocalTransform * SceneActor->GetActorTransform();
}
