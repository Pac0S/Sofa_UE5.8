// SofaTestActor.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SofaProceduralSurfaceComponent.h"
#include "SofaSceneSubsystem.h"
#include "SofaTestActor.generated.h"

UCLASS()
class SOFABRIDGE_API ASofaTestActor : public AActor
{
    GENERATED_BODY()
public:
    ASofaTestActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA Debug")
    bool bShowSofaDebug = true;

    UFUNCTION(BlueprintCallable, Category = "SOFA Debug")
    void SetSofaDebugVisible(bool bVisible);

    UFUNCTION(BlueprintPure, Category = "SOFA Debug")
    bool IsSofaDebugVisible() const { return bShowSofaDebug; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA Test Motion")
    bool bEnablePeriodicMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA Test Motion")
    FName MotionTargetId = TEXT("Liver01.mstate");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA Test Motion")
    FVector MotionAxis = FVector(0.f, 0.f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA Test Motion")
    float MotionAmplitudeCm = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA Test Motion")
    float MotionFrequencyHz = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOFA Test Motion")
    FTransform MotionBaseTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA Test")
    bool bAutoStartSimulation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA Test")
    bool bUseExplicitSceneFilePath = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA Test", meta=(EditCondition="bUseExplicitSceneFilePath"))
    FString SceneFilePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA Test", meta=(EditCondition="!bUseExplicitSceneFilePath"))
    FString SceneName = TEXT("liver");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA Test")
    FString ExternalScenesDirectory = TEXT("C:/Users/Pakito/Documents/Projets/Anisim/SofaScenes");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOFA Test")
    FString RelativeScenesDirectory = TEXT("SofaScenes");

    UFUNCTION(BlueprintCallable, Category="Sofa")
    bool GetObjectMaterialPath(FName ObjectId, FString& OutMaterialPath) const;

private:
    void DrawSofaDebug(const FSofaObjectState& ObjState);
    void DrawDebugPointsActorSpace(const FSofaObjectState& ObjState);
    void DrawDebugSurfaceActorSpace(const FSofaObjectState& ObjState);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOFA")
    TObjectPtr<USofaProceduralSurfaceComponent> ProceduralSurfaceComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USofaSceneSubsystem> SofaSubsystem = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOFA")
    FName VisualizedObjectId = TEXT("Liver01");
};