#pragma once

#include "CoreMinimal.h"
#include "SofaRuntimeTypes.h"

struct FSofaRuntimeScene;
struct FSofaRuntimeObjectDescriptor;


namespace SofaMaterialUtils
{
    struct FSofaParsedMtl
    {
        FString MaterialName;

        FLinearColor Ka = FLinearColor::Black;
        FLinearColor Kd = FLinearColor::White;
        FLinearColor Ks = FLinearColor::Black;
        FLinearColor Tf = FLinearColor::White;

        float Ns = 0.0f;
        float Ni = 1.0f;
        float D = 1.0f;
        int32 Illum = 0;

        FString MapKd;

        bool bHasKa = false;
        bool bHasKd = false;
        bool bHasKs = false;
        bool bHasTf = false;
        bool bHasNs = false;
        bool bHasNi = false;
        bool bHasD = false;
        bool bHasIllum = false;
        bool bHasMapKd = false;
    };

    bool ExtractMaterialDataFromMtl(
        const FString& MaterialPath,
        FSofaParsedMtl& OutMtl,
        FString& OutError);
}

namespace SofaCoordinateSystem 
{
    FQuat SofaLocalToUnrealLocalRotation(const FQuat& InSofaRotation);
    FQuat UnrealLocalToSofaLocalRotation(const FQuat& InUnrealRotation);
    FTransform SofaLocalToUnrealLocalTransform(const FTransform& InSofaLocalTransform, float InScale, FVector InScale3D);
    FTransform UnrealLocalToSofaLocalTransform(const FTransform& InUnrealLocalTransform, float InScale, FVector InScale3D);
}

namespace sofa::simulation
{
    class Node;
}
struct FSofaResolvedBinding;

namespace SofaSceneExtractor
{
    bool ExtractMechanicalDebugPoints(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        const FSofaResolvedBinding& MechanicalBinding,
        TArray<FSofaDebugPoint>& OutPoints,
        TArray<FSofaDebugPoint>& OutCollisionPoints,
        FString& OutError);
    
    bool ExtractMechanicalSurfaceDebugTriangles(
        const FSofaRuntimeScene& Scene,
        const FSofaResolvedBinding& SurfaceBinding,
        TArray<FSofaDebugTriangle>& OutTriangles,
        FString& OutError);

    bool ExtractVisualSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        const FSofaResolvedBinding& VisualBinding,
        FSofaSurfaceMeshState& OutMesh,
        FString& OutError);

    bool ExtractRenderableSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        const FSofaResolvedBinding& Binding,
        FSofaObjectState& OutState,
        FString& OutError);

    bool ExtractStaticCollisionDebugPoints(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        const FSofaResolvedBinding& Binding,
        TArray<FSofaDebugPoint>& OutPoints,
        FString& OutError);
}

namespace SofaFinder
{
}