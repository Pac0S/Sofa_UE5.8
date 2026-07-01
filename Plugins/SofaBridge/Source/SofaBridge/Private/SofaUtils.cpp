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
        return RuntimeObj.UnrealObjectTransform.TransformPosition(UnrealLocalPosition);
    }

    FVector UnrealToSofaPosition(
        const FVector& InUnrealPosition,
        const FSofaRuntimeObjectDescriptor& RuntimeObj)
    {
        const float SafeScale = FMath::IsNearlyZero(RuntimeObj.SofaScale) ? 1.0f : RuntimeObj.SofaScale;
        const FVector UnrealLocalPosition = RuntimeObj.UnrealObjectTransform.InverseTransformPosition(InUnrealPosition);
        const FVector SofaScaled(UnrealLocalPosition.X, UnrealLocalPosition.Z, UnrealLocalPosition.Y);

        return FVector(SofaScaled.X / SafeScale, SofaScaled.Y / SafeScale, SofaScaled.Z / SafeScale);
    }
}

namespace SofaSceneExtractor
{
    DEFINE_LOG_CATEGORY_STATIC(LogSofaSceneExtractor, Log, All);

    static void LogChildNodes(const sofa::simulation::Node::SPtr& Node)
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

    static sofa::simulation::Node* FindRuntimeObjectNode(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        FString& OutError)
    {
        auto* Root = Scene.RootNode.get();
        if (!Root)
        {
            OutError = TEXT("Root node is null");
            return nullptr;
        }

        sofa::simulation::Node* ObjectNode = Root->getChild(TCHAR_TO_UTF8(*RuntimeObj.SimulationNodeName));
        if (!ObjectNode)
        {
            OutError = FString::Printf(
                TEXT("Missing SOFA child node '%s'"),
                *RuntimeObj.SimulationNodeName);
            return nullptr;
        }

        return ObjectNode;
    }

    bool ExtractMechanicalDebugPoints(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        TArray<FSofaDebugPoint>& OutPoints,
        FString& OutError)
    {
        OutPoints.Reset();

        sofa::simulation::Node* ObjectNode = FindRuntimeObjectNode(Scene, RuntimeObj, OutError);
        if (!ObjectNode)
        {
            UE_LOG(LogSofaSceneExtractor, Error, TEXT("%s"), *OutError);
            return false;
        }

        sofa::core::objectmodel::BaseObject* Base =
            ObjectNode->getObject(TCHAR_TO_UTF8(*RuntimeObj.MechanicalObjectName));

        if (!Base)
        {
            OutError = FString::Printf(
                TEXT("Missing object '%s' in node '%s'"),
                *RuntimeObj.MechanicalObjectName,
                *RuntimeObj.SimulationNodeName);
            UE_LOG(LogSofaSceneExtractor, Error, TEXT("%s"), *OutError);
            return false;
        }

        auto* MState =
            dynamic_cast<sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>*>(Base);

        if (!MState)
        {
            OutError = FString::Printf(
                TEXT("Object '%s' in node '%s' is not a MechanicalObject<Vec3d>"),
                *RuntimeObj.MechanicalObjectName,
                *RuntimeObj.SimulationNodeName);
            UE_LOG(LogSofaSceneExtractor, Error, TEXT("%s"), *OutError);
            return false;
        }

        const auto& Positions = MState->readPositions();
        OutPoints.Reserve((int32)Positions.size());

        for (int32 i = 0; i < (int32)Positions.size(); ++i)
        {
            const auto& P = Positions[i];

            FSofaDebugPoint& Pt = OutPoints.AddDefaulted_GetRef();
            FVector SofaPos(P[0], P[1], P[2]);
            Pt.Position = SofaCoordinateSystem::SofaToUnrealPosition(SofaPos, RuntimeObj);
            Pt.Color = FColor::Green;
            Pt.Size = 8.0f;
        }

        return true;
    }

    bool ExtractMechanicalSurfaceDebugTriangles(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        TArray<FSofaDebugTriangle>& OutTriangles,
        FString& OutError)
    {
        OutTriangles.Reset();

        sofa::simulation::Node* ObjectNode = FindRuntimeObjectNode(Scene, RuntimeObj, OutError);
        if (!ObjectNode)
        {
            return false;
        }

        sofa::simulation::Node* SurfaceNode =
            ObjectNode->getChild(TCHAR_TO_UTF8(*RuntimeObj.SurfaceNodeName));

        if (!SurfaceNode)
        {
            OutError = FString::Printf(
                TEXT("Surface child '%s' not found under '%s'"),
                *RuntimeObj.SurfaceNodeName,
                *RuntimeObj.SimulationNodeName);
            return false;
        }

        sofa::core::objectmodel::BaseObject* SurfaceTopo =
            SurfaceNode->getObject(TCHAR_TO_UTF8(*RuntimeObj.SurfaceTopologyName));

        if (!SurfaceTopo)
        {
            OutError = FString::Printf(
                TEXT("Surface topology '%s' not found under '%s/%s'"),
                *RuntimeObj.SurfaceTopologyName,
                *RuntimeObj.SimulationNodeName,
                *RuntimeObj.SurfaceNodeName);
            return false;
        }

        auto* MeshTopology = dynamic_cast<sofa::core::topology::BaseMeshTopology*>(SurfaceTopo);
        if (!MeshTopology)
        {
            OutError = FString::Printf(
                TEXT("Object '%s' is not a BaseMeshTopology"),
                *RuntimeObj.SurfaceTopologyName);
            return false;
        }

        const sofa::Size TriangleCount = MeshTopology->getNbTriangles();
        OutTriangles.Reserve((int32)TriangleCount);

        for (sofa::Size i = 0; i < TriangleCount; ++i)
        {
            const auto& T = MeshTopology->getTriangle(i);

            FSofaDebugTriangle Tri;
            Tri.A = (int32)T[0];
            Tri.B = (int32)T[1];
            Tri.C = (int32)T[2];

            OutTriangles.Add(Tri);
        }

        return true;
    }

    bool ExtractVisualSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        FSofaSurfaceMeshState& OutMesh,
        FString& OutError)
    {
        OutMesh.Source = ESofaSurfaceSource::None;
        OutMesh.Vertices.Reset();
        OutMesh.Triangles.Reset();
        OutMesh.Normals.Reset();
        OutMesh.UV0.Reset();

        sofa::simulation::Node* ObjectNode = FindRuntimeObjectNode(Scene, RuntimeObj, OutError);
        if (!ObjectNode)
        {
            return false;
        }

        sofa::simulation::Node* VisualNode =
            ObjectNode->getChild(TCHAR_TO_UTF8(*RuntimeObj.VisualNodeName));

        if (!VisualNode)
        {
            OutError = FString::Printf(
                TEXT("Visual child node '%s' not found under '%s'"),
                *RuntimeObj.VisualNodeName,
                *RuntimeObj.SimulationNodeName);
            return false;
        }

        sofa::core::objectmodel::BaseObject* BaseMState =
            VisualNode->getObject(TCHAR_TO_UTF8(*RuntimeObj.VisualMechanicalObjectName));

        if (!BaseMState)
        {
            BaseMState = VisualNode->getObject("mstate");
        }

        if (!BaseMState)
        {
            OutError = FString::Printf(
                TEXT("Missing visual MechanicalObject '%s' or fallback 'mstate' in node '%s/%s'"),
                *RuntimeObj.VisualMechanicalObjectName,
                *RuntimeObj.SimulationNodeName,
                *RuntimeObj.VisualNodeName);
            return false;
        }

        auto* MState =
            dynamic_cast<sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>*>(BaseMState);

        if (!MState)
        {
            OutError = TEXT("Failed to cast visual MechanicalObject<Vec3d>");
            return false;
        }

        sofa::core::objectmodel::BaseObject* TopoObj =
            VisualNode->getObject(TCHAR_TO_UTF8(*RuntimeObj.VisualTopologyName));

        if (!TopoObj)
        {
            TopoObj = VisualNode->getObject("topo");
        }

        if (!TopoObj)
        {
            OutError = FString::Printf(
                TEXT("Missing visual topology '%s' or fallback 'topo' in node '%s/%s'"),
                *RuntimeObj.VisualTopologyName,
                *RuntimeObj.SimulationNodeName,
                *RuntimeObj.VisualNodeName);
            return false;
        }

        auto* MeshTopology = dynamic_cast<sofa::core::topology::BaseMeshTopology*>(TopoObj);
        if (!MeshTopology)
        {
            OutError = TEXT("Visual topology cast to BaseMeshTopology failed");
            return false;
        }

        const auto& Positions = MState->readPositions();
        OutMesh.Vertices.Reserve((int32)Positions.size());

        for (int32 i = 0; i < (int32)Positions.size(); ++i)
        {
            FVector SofaPos(Positions[i][0], Positions[i][1], Positions[i][2]);
            OutMesh.Vertices.Add(SofaCoordinateSystem::SofaToUnrealPosition(SofaPos, RuntimeObj));
        }

        const sofa::Size TriangleCount = MeshTopology->getNbTriangles();
        OutMesh.Triangles.Reserve((int32)TriangleCount * 3);

        for (sofa::Size i = 0; i < TriangleCount; ++i)
        {
            const auto& T = MeshTopology->getTriangle(i);
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
    }

    bool ExtractRenderableSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaRuntimeObjectDescriptor& RuntimeObj,
        FSofaObjectState& OutState,
        FString& OutError)
    {
        OutState.SurfaceMesh.Source = ESofaSurfaceSource::None;
        OutState.SurfaceMesh.Vertices.Reset();
        OutState.SurfaceMesh.Triangles.Reset();
        OutState.SurfaceMesh.Normals.Reset();
        OutState.SurfaceMesh.UV0.Reset();
        OutState.DebugPoints.Reset();
        OutState.SurfaceTriangles.Reset();

        FString VisualError;
        FString FallbackError;

        if (!ExtractMechanicalDebugPoints(Scene, RuntimeObj, OutState.DebugPoints, FallbackError))
        {
            OutError = FString::Printf(
                TEXT("Mechanical debug point extraction failed: %s"),
                *FallbackError);
            return false;
        }

        if (!ExtractMechanicalSurfaceDebugTriangles(Scene, RuntimeObj, OutState.SurfaceTriangles, FallbackError))
        {
            OutError = FString::Printf(
                TEXT("Mechanical surface triangle extraction failed: %s"),
                *FallbackError);
            return false;
        }

        if (RuntimeObj.bPreferVisualSurface &&
            ExtractVisualSurfaceMesh(Scene, RuntimeObj, OutState.SurfaceMesh, VisualError))
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