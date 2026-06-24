#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "SofaSceneTypes.h"

#include "SofaProceduralSurfaceComponent.generated.h"



UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOFABRIDGE_API USofaProceduralSurfaceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USofaProceduralSurfaceComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="SOFA")
    void UpdateMeshFromSofaState(const FSofaObjectState& State);

    UFUNCTION(BlueprintCallable, Category="SOFA")
    void ClearMesh();

    UFUNCTION(BlueprintPure, Category="SOFA")
    UProceduralMeshComponent* GetProceduralMesh() const
    {
        return ProceduralMesh;
    }

    UFUNCTION(BlueprintCallable, Category="SOFA")
    void SetMaterialPath(const FString& InMaterialPath);

    UFUNCTION(BlueprintCallable, Category="SOFA")
    bool InitializeMaterial();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOFA")
    TObjectPtr<UProceduralMeshComponent> ProceduralMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOFA")
    bool bCreateCollision = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOFA")
    bool bCastShadow = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOFA")
    bool bDoubleSidedGeometry = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOFA")
    FName ProceduralMeshName = TEXT("SofaProceduralMesh");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA")
    FString MaterialPath;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOFA")
    TObjectPtr<UMaterialInterface> BaseMaterial = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial = nullptr;

private:
    void EnsureProceduralMesh();

    static bool BuildProcMeshBuffers(
        const FSofaObjectState& State,
        TArray<FVector>& OutVertices,
        TArray<int32>& OutTriangles,
        TArray<FVector>& OutNormals,
        TArray<FVector2D>& OutUV0,
        TArray<FColor>& OutVertexColors,
        TArray<FProcMeshTangent>& OutTangents);

    static void ComputeNormals(
        const TArray<FVector>& Vertices,
        const TArray<int32>& Triangles,
        TArray<FVector>& OutNormals);

    static void ComputeTangents(
        const TArray<FVector>& Normals,
        TArray<FProcMeshTangent>& OutTangents);
};