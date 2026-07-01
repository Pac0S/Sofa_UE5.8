#pragma once

#include "CoreMinimal.h"
#include "SofaSceneTypes.h"
#include "SofaSceneConfig.h"

struct FSofaRuntimeScene;

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
    FVector SofaToUnrealPosition(const FVector& InPosition, const FSofaDeformableObjectConfig& Config);
    FVector UnrealToSofaPosition(const FVector& InPosition, const FSofaDeformableObjectConfig& Config);
}

namespace SofaSceneExtractor
{
    bool ExtractMechanicalDebugPoints(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        TArray<FSofaDebugPoint>& OutPoints,
        FString& OutError);
    
    bool ExtractMechanicalSurfaceDebugTriangles(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        TArray<FSofaDebugTriangle>& OutTriangles,
        FString& OutError);

    bool ExtractVisualSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        FSofaSurfaceMeshState& OutMesh,
        FString& OutError);

    bool ExtractRenderableSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        FSofaObjectState& OutState,
        FString& OutError);
}