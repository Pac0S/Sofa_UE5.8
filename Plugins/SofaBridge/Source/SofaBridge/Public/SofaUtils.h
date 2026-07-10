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
    FVector SofaToUnrealPosition(const FVector& InSofaPosition,const FSofaRuntimeObjectDescriptor& RuntimeObj);
    FVector UnrealToSofaPosition(const FVector& InUnrealPosition, const FSofaRuntimeObjectDescriptor& RuntimeObj);
    FVector UnrealToolPoseToSofaPosition(const FTransform& InUnrealPose, const FSofaRuntimeToolDescriptor& ToolDesc);
    FVector SofaToolPoseToUnrealPosition(const FTransform& InSofaPose, const FSofaRuntimeToolDescriptor& ToolDesc);
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
}

namespace SofaFinder
{
}