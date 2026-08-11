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
    FQuat SofaLocalToUnrealLocalRotation(const FQuat& InSofaRotation)
    {
        const FVector SofaX = InSofaRotation.RotateVector(FVector::ForwardVector); // (1,0,0)
        const FVector SofaY = InSofaRotation.RotateVector(FVector::RightVector);   // (0,1,0)
        const FVector SofaZ = InSofaRotation.RotateVector(FVector::UpVector);      // (0,0,1)
        const FVector UnrealX(SofaX.X, SofaX.Z, SofaX.Y);
        const FVector UnrealY(SofaY.X, SofaY.Z, SofaY.Y);
        const FVector UnrealZ(SofaZ.X, SofaZ.Z, SofaZ.Y);
        const FMatrix UnrealBasis(FPlane(UnrealX.X, UnrealX.Y, UnrealX.Z, 0.f), FPlane(UnrealY.X, UnrealY.Y, UnrealY.Z, 0.f), FPlane(UnrealZ.X, UnrealZ.Y, UnrealZ.Z, 0.f), FPlane(0.f, 0.f, 0.f, 1.f));
        return FQuat(UnrealBasis);
    }

    FQuat UnrealLocalToSofaLocalRotation(const FQuat& InUnrealRotation)
    {
        const FVector UnrealX = InUnrealRotation.RotateVector(FVector::ForwardVector); // (1,0,0)
        const FVector UnrealY = InUnrealRotation.RotateVector(FVector::RightVector);   // (0,1,0)
        const FVector UnrealZ = InUnrealRotation.RotateVector(FVector::UpVector);      // (0,0,1)
        const FVector SofaX(UnrealX.X, UnrealX.Z, UnrealX.Y);
        const FVector SofaY(UnrealY.X, UnrealY.Z, UnrealY.Y);
        const FVector SofaZ(UnrealZ.X, UnrealZ.Z, UnrealZ.Y);
        const FMatrix SofaBasis(FPlane(SofaX.X, SofaX.Y, SofaX.Z, 0.f), FPlane(SofaY.X, SofaY.Y, SofaY.Z, 0.f), FPlane(SofaZ.X, SofaZ.Y, SofaZ.Z, 0.f), FPlane(0.f, 0.f, 0.f, 1.f));
        return FQuat(SofaBasis);
    }

    FTransform SofaLocalToUnrealLocalTransform(const FTransform& InSofaLocalTransform, float InScale, FVector InScale3D)
    {
        const float SafeScale = FMath::IsNearlyZero(InScale) ? 1.0f : InScale;
        const FVector SofaPos = InSofaLocalTransform.GetLocation();
        const FVector UnrealPos(SofaPos.X * SafeScale, SofaPos.Z * SafeScale, SofaPos.Y * SafeScale);
        const FQuat UnrealRot = SofaLocalToUnrealLocalRotation(InSofaLocalTransform.GetRotation());
        const FVector SofaScale3D = InSofaLocalTransform.GetScale3D();
        const FVector UnrealScale3D(SofaScale3D.X, SofaScale3D.Z, SofaScale3D.Y);
        return FTransform(UnrealRot, UnrealPos, UnrealScale3D);
    }

    FTransform UnrealLocalToSofaLocalTransform(const FTransform& InUnrealLocalTransform, float InScale, FVector InScale3D)
    {
        const float SafeScale = FMath::IsNearlyZero(InScale) ? 1.0f : InScale;

        const FVector UnrealPos = InUnrealLocalTransform.GetLocation();
        const FVector SofaPos(UnrealPos.X / SafeScale, UnrealPos.Z / SafeScale, UnrealPos.Y / SafeScale);
        const FQuat SofaRot = UnrealLocalToSofaLocalRotation(InUnrealLocalTransform.GetRotation());
        const FVector UnrealScale3D = InUnrealLocalTransform.GetScale3D();
        const FVector SofaScale3D(UnrealScale3D.X, UnrealScale3D.Z, UnrealScale3D.Y);
        return FTransform(SofaRot, SofaPos, SofaScale3D);
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

        using Vec3MechanicalObject = sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>;

        if (auto* MO = dynamic_cast<Vec3MechanicalObject*>(Object))
        {
            const auto& Positions = MO->readPositions();
            OutSofaPositions.Reserve((int32)Positions.size());

            for (int32 i = 0; i < (int32)Positions.size(); ++i)
            {
                const auto& P = Positions[i];
                OutSofaPositions.Add(FVector((float)P[0], (float)P[1], (float)P[2]));
            }

            if (OutSofaPositions.Num() > 0) return true;
        }

        using RigidMechanicalObject = sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Rigid3Types>;

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

            if (OutSofaPositions.Num() > 0) return true;
        }

        using Vec3Data = sofa::core::objectmodel::Data<sofa::type::vector<sofa::type::Vec3d>>;

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

                if (OutSofaPositions.Num() > 0) return true;
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
        TArray<FSofaDebugPoint>& OutCollisionPoints,
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
            const FTransform SofaLocalObjectTransform(FQuat::Identity, SofaPos);
            const FTransform UnrealLocalObjectTransform = SofaCoordinateSystem::SofaLocalToUnrealLocalTransform(SofaLocalObjectTransform, RuntimeObj.SofaScale, RuntimeObj.SofaScale3D);
            /*if (OutPoints.Num() < 5)
            {
                UE_LOG(LogTemp, Verbose,
                    TEXT("[SOFA][MechanicalDebug] Idx=%d SofaPos=%s UnrealPos=%s Scale=%.3f"),
                    OutPoints.Num(),
                    *SofaPos.ToString(),
                    *UnrealLocalObjectTransform.GetLocation().ToString());
            }*/
            Pt.Position = UnrealLocalObjectTransform.GetLocation();
            Pt.Color = FColor::Green;
            Pt.Size = 8.0f;
        }

        sofa::core::objectmodel::BaseObject* CollisionMechanicalObject = SofaFinder::ResolveObjectByKey(Scene, Binding.Descriptor.CollisionObjectKey);

        if (!CollisionMechanicalObject)
        {
            OutError = FString::Printf(TEXT("Collision debug points: failed to resolve object '%s'"), *Binding.Descriptor.CollisionObjectKey);
            return false;
        }

        TArray<FVector> CollisionPositions;
        if (!ReadVec3PositionsFromBaseObject(CollisionMechanicalObject, CollisionPositions))
        {
            OutError = FString::Printf(
                TEXT("MechanicalObject: object '%s' does not expose readable positions"),
                *Binding.Descriptor.ObjectKey);
            return false;
        }
        OutCollisionPoints.Reserve(CollisionPositions.Num());

        for (const FVector& SofaPos : CollisionPositions)
        {
            const FTransform SofaLocalObjectTransform(FQuat::Identity, SofaPos);
            const FTransform UnrealLocalObjectTransform = SofaCoordinateSystem::SofaLocalToUnrealLocalTransform(SofaLocalObjectTransform, RuntimeObj.SofaScale, RuntimeObj.SofaScale3D);
            FSofaDebugPoint& Pt = OutCollisionPoints.AddDefaulted_GetRef();
            if (OutCollisionPoints.Num() < 5)
            {
                UE_LOG(LogTemp, Verbose,
                    TEXT("[SOFA][CollisionObject] Idx=%d SofaPos=%s UnrealPos=%s"),
                    OutCollisionPoints.Num(),
                    *SofaPos.ToString(),
                    *UnrealLocalObjectTransform.GetLocation().ToString());
            }
            Pt.Position = UnrealLocalObjectTransform.GetLocation();
            Pt.Color = FColor::Blue;
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
            const FTransform SofaLocalObjectTransform(FQuat::Identity, SofaPos);
            const FTransform UnrealLocalObjectTransform = SofaCoordinateSystem::SofaLocalToUnrealLocalTransform(SofaLocalObjectTransform, RuntimeObj.SofaScale, RuntimeObj.SofaScale3D);
            /*if (OutMesh.Vertices.Num() < 5)
            {
                UE_LOG(LogTemp, Verbose,
                    TEXT("[SOFA][VisualSurface] Idx=%d SofaPos=%s UnrealPos=%s"),
                    OutMesh.Vertices.Num(),
                    *SofaPos.ToString(),
                    *UnrealLocalObjectTransform.GetLocation().ToString());
            }*/
            OutMesh.Vertices.Add(UnrealLocalObjectTransform.GetLocation());
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
        OutState.CollisionDebugPoints.Reset();
        OutState.SurfaceTriangles.Reset();

        FString VisualError;
        FString FallbackError;

        if (!ExtractMechanicalDebugPoints(
            Scene,
            RuntimeObj,
            Binding,
            OutState.DebugPoints,
            OutState.CollisionDebugPoints,
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

    bool ExtractStaticCollisionDebugPoints(
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
            OutError = TEXT("Static collision: invalid binding");
            return false;
        }

        if (Binding.Descriptor.ObjectKey.IsEmpty())
        {
            OutError = TEXT("Static collision: missing object mechanical key");
            return false;
        }

        sofa::core::objectmodel::BaseObject* CollisionObject = SofaFinder::ResolveObjectByKey(Scene, Binding.Descriptor.CollisionObjectKey);

        UE_LOG(LogTemp, Log, TEXT("[ExtractStaticCollisionDebugPoints] Object key : %s"), *Binding.Descriptor.ObjectKey);

        if (!CollisionObject)
        {
            OutError = FString::Printf(
                TEXT("Static collision: failed to resolve object '%s'"),
                *Binding.Descriptor.ObjectKey);
            return false;
        }

        TArray<FVector> SofaPositions;
        if (!ReadVec3PositionsFromBaseObject(CollisionObject, SofaPositions))
        {
            OutError = FString::Printf(
                TEXT("Static collision: object '%s' does not expose readable positions"),
                *Binding.Descriptor.ObjectKey);
            return false;
        }

        if (SofaPositions.IsEmpty())
        {
            OutError = FString::Printf(
                TEXT("Static collision: object '%s' exposes zero positions"),
                *Binding.Descriptor.ObjectKey);
            return false;
        }

        OutPoints.Reserve(SofaPositions.Num());

        for (const FVector& SofaPos : SofaPositions)
        {
            const FTransform SofaLocalObjectTransform(FQuat::Identity, SofaPos);
            const FTransform UnrealLocalObjectTransform = SofaCoordinateSystem::SofaLocalToUnrealLocalTransform(SofaLocalObjectTransform, RuntimeObj.SofaScale, RuntimeObj.SofaScale3D);
            FSofaDebugPoint& Pt = OutPoints.AddDefaulted_GetRef();
            UE_LOG(LogTemp, Warning,
                TEXT("[SOFA][StaticObject] Idx=%d SofaPos=%s UnrealPos=%s"),
                OutPoints.Num(),
                *SofaPos.ToString(),
                *UnrealLocalObjectTransform.GetLocation().ToString());
            Pt.Position = UnrealLocalObjectTransform.GetLocation();
            Pt.Color = FColor::Green;
            Pt.Size = 6.0f;
        }

        return true;
#endif
    }
}

