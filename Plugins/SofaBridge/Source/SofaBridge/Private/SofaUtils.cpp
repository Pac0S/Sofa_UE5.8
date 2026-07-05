#include "SofaUtils.h"
#include "Misc/FileHelper.h"
#include "Math/UnrealMathUtility.h"
#include "SofaIncludes.h"
#include "SofaRuntimeScene.h"

namespace SofaMaterialUtils
{
    bool ExtractMaterialDataFromMtl(
        const FString& MaterialPath,
        FSofaParsedMtl& OutMtl,
        FString& OutError)
    {
        OutMtl = FSofaParsedMtl{};
        OutError.Reset();

        TArray<FString> Lines;
        if (!FFileHelper::LoadFileToStringArray(Lines, *MaterialPath))
        {
            OutError = FString::Printf(
                TEXT("ExtractMaterialDataFromMtl: failed to read MTL file '%s'"),
                *MaterialPath);
            return false;
        }

        auto TryParseVec3 = [](const FString& Value, FLinearColor& OutColor) -> bool
            {
                TArray<FString> Parts;
                Value.ParseIntoArray(Parts, TEXT(" "), true);
                if (Parts.Num() < 3)
                {
                    return false;
                }

                OutColor = FLinearColor(
                    FCString::Atof(*Parts[0]),
                    FCString::Atof(*Parts[1]),
                    FCString::Atof(*Parts[2]),
                    1.0f);

                return true;
            };

        auto TryParseFloat = [](const FString& Value, float& OutFloat) -> bool
            {
                if (Value.IsEmpty())
                {
                    return false;
                }

                OutFloat = FCString::Atof(*Value);
                return true;
            };

        auto TryParseInt = [](const FString& Value, int32& OutInt) -> bool
            {
                if (Value.IsEmpty())
                {
                    return false;
                }

                OutInt = FCString::Atoi(*Value);
                return true;
            };

        for (FString Line : Lines)
        {
            Line = Line.TrimStartAndEnd();

            if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
            {
                continue;
            }

            FString Key;
            FString Value;
            if (!Line.Split(TEXT(" "), &Key, &Value))
            {
                continue;
            }

            Key = Key.TrimStartAndEnd();
            Value = Value.TrimStartAndEnd();

            if (Key.Equals(TEXT("newmtl"), ESearchCase::IgnoreCase))
            {
                OutMtl.MaterialName = Value;
            }
            else if (Key.Equals(TEXT("Ka"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasKa = TryParseVec3(Value, OutMtl.Ka);
            }
            else if (Key.Equals(TEXT("Kd"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasKd = TryParseVec3(Value, OutMtl.Kd);
            }
            else if (Key.Equals(TEXT("Ks"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasKs = TryParseVec3(Value, OutMtl.Ks);
            }
            else if (Key.Equals(TEXT("Tf"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasTf = TryParseVec3(Value, OutMtl.Tf);
            }
            else if (Key.Equals(TEXT("Ns"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasNs = TryParseFloat(Value, OutMtl.Ns);
            }
            else if (Key.Equals(TEXT("Ni"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasNi = TryParseFloat(Value, OutMtl.Ni);
            }
            else if (Key.Equals(TEXT("d"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasD = TryParseFloat(Value, OutMtl.D);
            }
            else if (Key.Equals(TEXT("Tr"), ESearchCase::IgnoreCase))
            {
                float Transparency = 0.0f;
                if (TryParseFloat(Value, Transparency))
                {
                    OutMtl.D = 1.0f - Transparency;
                    OutMtl.bHasD = true;
                }
            }
            else if (Key.Equals(TEXT("illum"), ESearchCase::IgnoreCase))
            {
                OutMtl.bHasIllum = TryParseInt(Value, OutMtl.Illum);
            }
            else if (Key.Equals(TEXT("map_Kd"), ESearchCase::IgnoreCase))
            {
                OutMtl.MapKd = Value;
                OutMtl.bHasMapKd = !Value.IsEmpty();
            }
        }
        return true;
    }
}

namespace SofaCoordinateSystem
{
    FVector SofaToUnrealPosition(
        const FVector& InSofaPosition,
        const FSofaRuntimeObjectDescriptor& RuntimeObj)
    {
        const float SafeScale = FMath::IsNearlyZero(RuntimeObj.SofaScale) ? 1.0f : RuntimeObj.SofaScale;
        const FVector SofaScaled(InSofaPosition.X * SafeScale, InSofaPosition.Y * SafeScale, InSofaPosition.Z * SafeScale);
        const FVector UnrealLocalPosition(SofaScaled.X, SofaScaled.Z, SofaScaled.Y);
        return RuntimeObj.UnrealAnchorTransform.TransformPosition(UnrealLocalPosition);
    }

    FVector SofaToolPoseToUnrealPosition(
        const FTransform& InSofaPose,
        const FSofaRuntimeToolDescriptor& ToolDesc)
    {
        const float SafeScale = FMath::IsNearlyZero(ToolDesc.SofaScale) ? 1.0f : ToolDesc.SofaScale;
        const FVector SofaPosition = InSofaPose.GetLocation();
        const FVector UnrealLocalPosition(SofaPosition.X * SafeScale, SofaPosition.Z * SafeScale, SofaPosition.Y * SafeScale);

        return ToolDesc.UnrealAnchorTransform.TransformPosition(UnrealLocalPosition);
    }

    FVector UnrealToSofaPosition(
        const FVector& InUnrealPosition,
        const FSofaRuntimeObjectDescriptor& RuntimeObj)
    {
        const float SafeScale = FMath::IsNearlyZero(RuntimeObj.SofaScale) ? 1.0f : RuntimeObj.SofaScale;
        const FVector UnrealLocalPosition = RuntimeObj.UnrealAnchorTransform.InverseTransformPosition(InUnrealPosition);
        const FVector SofaScaled(UnrealLocalPosition.X, UnrealLocalPosition.Z, UnrealLocalPosition.Y);

        return FVector(SofaScaled.X / SafeScale, SofaScaled.Y / SafeScale, SofaScaled.Z / SafeScale);
    }

    FVector UnrealToolPoseToSofaPosition(
        const FTransform& InUnrealPose,
        const FSofaRuntimeToolDescriptor& ToolDesc)
    {
        const float SafeScale = FMath::IsNearlyZero(ToolDesc.SofaScale) ? 1.0f : ToolDesc.SofaScale;
        const FVector UnrealWorldPosition = InUnrealPose.GetLocation();
        const FVector UnrealLocalPosition = ToolDesc.UnrealAnchorTransform.InverseTransformPosition(UnrealWorldPosition);
        const FVector SofaScaled(UnrealLocalPosition.X, UnrealLocalPosition.Z,UnrealLocalPosition.Y);

        return FVector(SofaScaled.X / SafeScale, SofaScaled.Y / SafeScale, SofaScaled.Z / SafeScale);
    }
}

namespace SofaFinder
{
    static const FSofaIndexedNode* FindIndexedNodeByPath(
        const FSofaRuntimeScene& Scene,
        const FString& NodePath)
    {
        if (NodePath.IsEmpty())
        {
            return nullptr;
        }

        const FSofaIndexedNode* IndexedNode = Scene.NodeIndexByPath.Find(NodePath);
        if (!IndexedNode)
        {
            return nullptr;
        }

        if (IndexedNode->Generation != Scene.SceneGeneration)
        {
            return nullptr;
        }

        return IndexedNode;
    }

    static const FSofaIndexedObject* FindIndexedObjectByKey(
        const FSofaRuntimeScene& Scene,
        const FString& ObjectKey)
    {
        if (ObjectKey.IsEmpty())
        {
            return nullptr;
        }

        const FSofaIndexedObject* IndexedObject = Scene.ObjectIndexByKey.Find(ObjectKey);
        if (!IndexedObject)
        {
            return nullptr;
        }

        if (IndexedObject->Generation != Scene.SceneGeneration)
        {
            return nullptr;
        }

        return IndexedObject;
    }

    static sofa::simulation::Node* ResolveNodeByPathSegments(
        const FSofaRuntimeScene& Scene,
        const FString& NodePath)
    {
        if (!Scene.RootNode || NodePath.IsEmpty())
        {
            return nullptr;
        }

        if (NodePath == TEXT("root"))
        {
            return Scene.RootNode.get();
        }

        TArray<FString> Segments;
        NodePath.ParseIntoArray(Segments, TEXT("/"), true);

        if (Segments.Num() == 0 || Segments[0] != TEXT("root"))
        {
            return nullptr;
        }

        sofa::simulation::Node* Current = Scene.RootNode.get();

        for (int32 i = 1; i < Segments.Num(); ++i)
        {
            if (!Current)
            {
                return nullptr;
            }

            Current = Current->getChild(TCHAR_TO_UTF8(*Segments[i]));
        }

        return Current;
    }

    static sofa::core::objectmodel::BaseObject* ResolveObjectByKey(
        const FSofaRuntimeScene& Scene,
        const FString& ObjectKey)
    {
        const FSofaIndexedObject* IndexedObject = FindIndexedObjectByKey(Scene, ObjectKey);
        if (!IndexedObject)
        {
            return nullptr;
        }
        if (!FindIndexedNodeByPath(Scene, IndexedObject->NodePath))
        {
            return nullptr;
        }

        sofa::simulation::Node* Node = ResolveNodeByPathSegments(Scene, IndexedObject->NodePath);
        if (!Node)
        {
            return nullptr;
        }

        return Node->getObject(TCHAR_TO_UTF8(*IndexedObject->ObjectName));
    }

    static sofa::core::topology::BaseMeshTopology* ResolveTopologyByKey(
        const FSofaRuntimeScene& Scene,
        const FString& TopologyObjectKey)
    {
        sofa::core::objectmodel::BaseObject* Base = ResolveObjectByKey(Scene, TopologyObjectKey);

        return dynamic_cast<sofa::core::topology::BaseMeshTopology*>(Base);
    }
}

namespace SofaSceneExtractor
{
    DEFINE_LOG_CATEGORY_STATIC(LogSofaSceneExtractor, Log, All);

    static bool ReadVec3PositionsFromBaseObject(
        sofa::core::objectmodel::BaseObject* Object,
        TArray<FVector>& OutSofaPositions)
    {
        OutSofaPositions.Reset();

        if (!Object)
        {
            return false;
        }

        using Vec3MechanicalObject =
            sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>;

        if (auto* MO = dynamic_cast<Vec3MechanicalObject*>(Object))
        {
            const auto& Positions = MO->readPositions();
            OutSofaPositions.Reserve((int32)Positions.size());

            for (int32 i = 0; i < (int32)Positions.size(); ++i)
            {
                const auto& P = Positions[i];
                OutSofaPositions.Add(FVector(
                    static_cast<float>(P[0]),
                    static_cast<float>(P[1]),
                    static_cast<float>(P[2])));
            }

            return OutSofaPositions.Num() > 0;
        }

        using RigidMechanicalObject =
            sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Rigid3Types>;

        if (auto* MO = dynamic_cast<RigidMechanicalObject*>(Object))
        {
            const auto& Positions = MO->readPositions();
            OutSofaPositions.Reserve((int32)Positions.size());

            for (int32 i = 0; i < (int32)Positions.size(); ++i)
            {
                const auto& P = Positions[i];
                const auto& C = P.getCenter();

                OutSofaPositions.Add(FVector(
                    static_cast<float>(C[0]),
                    static_cast<float>(C[1]),
                    static_cast<float>(C[2])));
            }

            return OutSofaPositions.Num() > 0;
        }

        using Vec3Data =
            sofa::core::objectmodel::Data<sofa::type::vector<sofa::type::Vec3d>>;

        if (auto* PositionData = Object->findData("position"))
        {
            if (auto* TypedData = dynamic_cast<Vec3Data*>(PositionData))
            {
                const auto& Positions = TypedData->getValue();
                OutSofaPositions.Reserve((int32)Positions.size());

                for (int32 i = 0; i < (int32)Positions.size(); ++i)
                {
                    const auto& P = Positions[i];
                    OutSofaPositions.Add(FVector(
                        static_cast<float>(P[0]),
                        static_cast<float>(P[1]),
                        static_cast<float>(P[2])));
                }

                return OutSofaPositions.Num() > 0;
            }
        }

        return false;
    }

    void LogChildNodes(const sofa::simulation::Node::SPtr& Node)
    {
        if (!Node)
        {
            UE_LOG(LogSofaSceneExtractor, Warning, TEXT("NodeSPtr is null"));
            return;
        }

        auto* RawNode = Node.get();
        UE_LOG(LogSofaSceneExtractor, Warning, TEXT("node='%hs' ptr=%p"),
            RawNode->getName().c_str(),
            RawNode);

        const auto& Children = RawNode->getChildren();
        UE_LOG(LogSofaSceneExtractor, Warning, TEXT("child count=%d"),
            (int32)Children.size());

        int32 Index = 0;
        for (const auto& Child : Children)
        {
            if (Child)
            {
                UE_LOG(LogSofaSceneExtractor, Warning,
                    TEXT("child name='%hs' "), Child->getName().c_str());
            }else
            {
                UE_LOG(LogSofaSceneExtractor, Warning,
                    TEXT("invalid child at index % d"), Index);
            }
            ++Index;
            UE_LOG(LogSofaSceneExtractor, Warning, TEXT("index ++ = %i"), Index);
        }
        UE_LOG(LogSofaSceneExtractor, Warning, TEXT("end of child list"));
    }

    bool ExtractMechanicalDebugPoints(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        const FSofaResolvedBinding& Binding,
        TArray<FSofaDebugPoint>& OutPoints,
        FString& OutError)
    {
        OutPoints.Reset();
        OutError.Reset();

#if !SOFA_SDK_ENABLED
        OutError = TEXT("SOFA SDK disabled");
        return false;
#else
        if (!Binding.IsValidForGeneration(Scene.SceneGeneration))
        {
            OutError = TEXT("Mechanical debug points: invalid binding");
            return false;
        }

        if (Binding.Descriptor.ObjectKey.IsEmpty())
        {
            OutError = TEXT("Mechanical debug points: missing mechanical object key");
            return false;
        }

        sofa::core::objectmodel::BaseObject* MechanicalObject = SofaFinder::ResolveObjectByKey(Scene, Binding.Descriptor.ObjectKey);
        if (!MechanicalObject)
        {
            OutError = FString::Printf(
                TEXT("Mechanical debug points: failed to resolve object '%s'"),
                *Binding.Descriptor.ObjectKey);
            return false;
        }

        TArray<FVector> SofaPositions;
        if (!ReadVec3PositionsFromBaseObject(MechanicalObject, SofaPositions))
        {
            OutError = FString::Printf(
                TEXT("Mechanical debug points: object '%s' does not expose readable positions"),
                *Binding.Descriptor.ObjectKey);
            return false;
        }

        OutPoints.Reserve(SofaPositions.Num());

        for (const FVector& SofaPos : SofaPositions)
        {
            FSofaDebugPoint& Pt = OutPoints.AddDefaulted_GetRef();
            Pt.Position = SofaCoordinateSystem::SofaToUnrealPosition(SofaPos, RuntimeObj);
            Pt.Color = FColor::Green;
            Pt.Size = 8.0f;
        }

        return true;
#endif
    }

    bool ExtractMechanicalSurfaceDebugTriangles(
        const FSofaRuntimeScene& Scene,
        const FSofaResolvedBinding& Binding,
        TArray<FSofaDebugTriangle>& OutTriangles,
        FString& OutError)
    {
        OutTriangles.Reset();
        OutError.Reset();

#if !SOFA_SDK_ENABLED
        OutError = TEXT("SOFA SDK disabled");
        return false;
#else
        if (!Binding.IsValidForGeneration(Scene.SceneGeneration))
        {
            OutError = TEXT("Mechanical surface triangles: invalid binding");
            return false;
        }

        if (Binding.Descriptor.SurfaceTopologyObjectKey.IsEmpty())
        {
            OutError = TEXT("Mechanical surface triangles: missing surface topology object key");
            return false;
        }

        sofa::core::topology::BaseMeshTopology* MeshTopology = SofaFinder::ResolveTopologyByKey(Scene, Binding.Descriptor.SurfaceTopologyObjectKey);
        if (!MeshTopology)
        {
            OutError = FString::Printf(
                TEXT("Mechanical surface triangles: failed to resolve topology '%s'"),
                *Binding.Descriptor.SurfaceTopologyObjectKey);
            return false;
        }

        const sofa::Size TriangleCount = MeshTopology->getNbTriangles();
        OutTriangles.Reserve((int32)TriangleCount);

        for (sofa::Size i = 0; i < TriangleCount; ++i)
        {
            const auto& T = MeshTopology->getTriangle(i);

            FSofaDebugTriangle& Tri = OutTriangles.AddDefaulted_GetRef();
            Tri.A = (int32)T[0];
            Tri.B = (int32)T[1];
            Tri.C = (int32)T[2];
        }

        return true;
#endif
    }

    bool ExtractVisualSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        const FSofaResolvedBinding& Binding,
        FSofaSurfaceMeshState& OutMesh,
        FString& OutError)
    {
        OutMesh.Source = ESofaSurfaceSource::None;
        OutMesh.Vertices.Reset();
        OutMesh.Triangles.Reset();
        OutMesh.Normals.Reset();
        OutMesh.UV0.Reset();
        OutError.Reset();

#if !SOFA_SDK_ENABLED
        OutError = TEXT("SOFA SDK disabled");
        return false;
#else
        if (!Binding.IsValidForGeneration(Scene.SceneGeneration))
        {
            OutError = TEXT("Visual surface: invalid binding");
            return false;
        }

        if (Binding.Descriptor.VisualObjectKey.IsEmpty())
        {
            OutError = TEXT("Visual surface: missing visual object key");
            return false;
        }

        if (Binding.Descriptor.VisualTopologyObjectKey.IsEmpty())
        {
            OutError = TEXT("Visual surface: missing visual topology object key");
            return false;
        }

        sofa::core::objectmodel::BaseObject* VisualObject = SofaFinder::ResolveObjectByKey(Scene, Binding.Descriptor.VisualObjectKey);
        if (!VisualObject)
        {
            OutError = FString::Printf(
                TEXT("Visual surface: failed to resolve visual object '%s'"),
                *Binding.Descriptor.VisualObjectKey);
            return false;
        }

        sofa::core::topology::BaseMeshTopology* VisualTopology = SofaFinder::ResolveTopologyByKey(Scene, Binding.Descriptor.VisualTopologyObjectKey);
        if (!VisualTopology)
        {
            OutError = FString::Printf(
                TEXT("Visual surface: failed to resolve visual topology '%s'"),
                *Binding.Descriptor.VisualTopologyObjectKey);
            return false;
        }

        TArray<FVector> SofaPositions;
        if (!ReadVec3PositionsFromBaseObject(VisualObject, SofaPositions))
        {
            OutError = FString::Printf(
                TEXT("Visual surface: object '%s' does not expose readable positions"),
                *Binding.Descriptor.VisualObjectKey);
            return false;
        }

        OutMesh.Vertices.Reserve(SofaPositions.Num());
        for (const FVector& SofaPos : SofaPositions)
        {
            OutMesh.Vertices.Add(
                SofaCoordinateSystem::SofaToUnrealPosition(SofaPos, RuntimeObj));
        }

        const sofa::Size TriangleCount = VisualTopology->getNbTriangles();
        OutMesh.Triangles.Reserve((int32)TriangleCount * 3);

        for (sofa::Size i = 0; i < TriangleCount; ++i)
        {
            const auto& T = VisualTopology->getTriangle(i);
            OutMesh.Triangles.Add((int32)T[0]);
            OutMesh.Triangles.Add((int32)T[1]);
            OutMesh.Triangles.Add((int32)T[2]);
        }

        if (OutMesh.Vertices.Num() == 0)
        {
            OutError = TEXT("No vertices extracted from visual mesh");
            return false;
        }

        if (OutMesh.Triangles.Num() == 0)
        {
            OutError = TEXT("No triangles extracted from visual mesh");
            return false;
        }

        OutMesh.Source = ESofaSurfaceSource::VisualSurface;
        return true;
#endif
    }

    bool ExtractRenderableSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        const FSofaResolvedBinding& Binding,
        FSofaObjectState& OutState,
        FString& OutError)
    {
        OutError.Reset();

        OutState.SurfaceMesh.Source = ESofaSurfaceSource::None;
        OutState.SurfaceMesh.Vertices.Reset();
        OutState.SurfaceMesh.Triangles.Reset();
        OutState.SurfaceMesh.Normals.Reset();
        OutState.SurfaceMesh.UV0.Reset();
        OutState.DebugPoints.Reset();
        OutState.SurfaceTriangles.Reset();

        FString VisualError;
        FString FallbackError;

        if (!ExtractMechanicalDebugPoints(
            Scene,
            RuntimeObj,
            Binding,
            OutState.DebugPoints,
            FallbackError))
        {
            OutError = FString::Printf(
                TEXT("Mechanical debug point extraction failed: %s"),
                *FallbackError);
            return false;
        }

        if (!ExtractMechanicalSurfaceDebugTriangles(
            Scene,
            Binding,
            OutState.SurfaceTriangles,
            FallbackError))
        {
            OutError = FString::Printf(
                TEXT("Mechanical surface triangle extraction failed: %s"),
                *FallbackError);
            return false;
        }

        if (RuntimeObj.bPreferVisualSurface &&
            ExtractVisualSurfaceMesh(
                Scene,
                RuntimeObj,
                Binding,
                OutState.SurfaceMesh,
                VisualError))
        {
            OutState.SurfaceMesh.Source = ESofaSurfaceSource::VisualSurface;
            return true;
        }

        if (OutState.DebugPoints.Num() == 0)
        {
            OutError = FString::Printf(
                TEXT("Fallback produced no vertices (visual error: %s)"),
                *VisualError);
            return false;
        }

        if (OutState.SurfaceTriangles.Num() == 0)
        {
            OutError = FString::Printf(
                TEXT("Fallback produced no triangles (visual error: %s)"),
                *VisualError);
            return false;
        }

        OutState.SurfaceMesh.Vertices.Reserve(OutState.DebugPoints.Num());
        for (const FSofaDebugPoint& Pt : OutState.DebugPoints)
        {
            OutState.SurfaceMesh.Vertices.Add(Pt.Position);
        }

        OutState.SurfaceMesh.Triangles.Reserve(OutState.SurfaceTriangles.Num() * 3);
        for (const FSofaDebugTriangle& Tri : OutState.SurfaceTriangles)
        {
            OutState.SurfaceMesh.Triangles.Add(Tri.A);
            OutState.SurfaceMesh.Triangles.Add(Tri.B);
            OutState.SurfaceMesh.Triangles.Add(Tri.C);
        }

        OutState.SurfaceMesh.Source = ESofaSurfaceSource::DerivedSurface;
        return true;
    }
}

