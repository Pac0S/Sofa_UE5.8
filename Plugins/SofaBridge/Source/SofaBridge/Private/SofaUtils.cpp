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
    FVector SofaToUnrealPosition(const FVector& InPosition, const FSofaDeformableObjectConfig& Config)
    {
        // X = lateral / Y = vertical / Z = depth
        // to Unreal : X forward, Y right, Z up
        const FVector UEPos(
            InPosition.X * Config.Scale + Config.Translation.X,
            InPosition.Z * Config.Scale + Config.Translation.Y,
            InPosition.Y * Config.Scale + Config.Translation.Z);

        return UEPos;
    }

    FVector UnrealToSofaPosition(const FVector& InPosition, const FSofaDeformableObjectConfig& Config)
    {
        const FVector SofaPos(
            (InPosition.X - Config.Translation.X) / Config.Scale,
            (InPosition.Z - Config.Translation.Y) / Config.Scale,
            (InPosition.Y - Config.Translation.Z) / Config.Scale);
        return SofaPos;
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

    bool ExtractMechanicalDebugPoints(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        TArray<FSofaDebugPoint>& OutPoints,
        FString& OutError)
    {
        OutPoints.Reset();

        auto* Root = Scene.RootNode.get();
        if (!Root)
        {
            OutError = TEXT("Root node is null");
            UE_LOG(LogSofaSceneExtractor, Error, TEXT("%s"), *OutError);
            return false;
        }

        //UE_LOG(LogSofaSceneExtractor, Warning, TEXT("Extract: root ok, looking for node '%s'"), *Config.Id);

        auto* ObjectNode = Root->getChild(TCHAR_TO_UTF8(*Config.Id));
        if (!ObjectNode)
        {
            OutError = FString::Printf(TEXT("Missing SOFA child node '%s'"), *Config.Id);
            UE_LOG(LogSofaSceneExtractor, Error, TEXT("%s"), *OutError);
            return false;
        }

        sofa::core::objectmodel::BaseObject* Base = ObjectNode->getObject("mstate");
        if (!Base)
        {
            OutError = FString::Printf(TEXT("Missing object 'mstate' in node '%s'"), *Config.Id);
            UE_LOG(LogSofaSceneExtractor, Error, TEXT("%s"), *OutError);
            return false;
        }

        auto* MState = dynamic_cast<sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>*>(Base);


        const auto& Positions = MState->readPositions();

        for (int32 i = 0; i < (int32)Positions.size(); ++i)
        {
            const auto& P = Positions[i];

            FSofaDebugPoint& Pt = OutPoints.AddDefaulted_GetRef();
            FVector Pos(P[0], P[1], P[2]);
            Pt.Position = SofaCoordinateSystem::SofaToUnrealPosition(Pos, Config);
            Pt.Color = FColor::Green;
            Pt.Size = 8.0f;
        }

        return true;
    }

    bool ExtractMechanicalSurfaceDebugTriangles(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        TArray<FSofaDebugTriangle>& OutTriangles,
        FString& OutError)
    {
        OutTriangles.Reset();

        if (!Scene.RootNode)
        {
            OutError = TEXT("ExtractSurfaceTriangles: invalid root node");
            return false;
        }

        sofa::simulation::Node* Root = Scene.RootNode.get();
        if (!Root)
        {
            OutError = TEXT("ExtractSurfaceTriangles: root node is null");
            return false;
        }

        const std::string ObjectId = TCHAR_TO_UTF8(*Config.Id);
        sofa::simulation::Node* ObjectNode = Root->getChild(ObjectId);

        /*UE_LOG(LogSofaSceneExtractor, Warning, TEXT("ExtractSurfaceTriangles: looking for object '%s' => %p"),
            *Config.Id, ObjectNode);*/

        if (!ObjectNode)
        {
            OutError = FString::Printf(TEXT("ExtractSurfaceTriangles: object node '%s' not found"), *Config.Id);
            return false;
        }

        sofa::simulation::Node* SurfaceNode = ObjectNode->getChild("Surface");

        /*UE_LOG(LogSofaSceneExtractor, Warning, TEXT("ExtractSurfaceTriangles: Surface node => %p"),
            SurfaceNode);*/

        if (!SurfaceNode)
        {
            OutError = FString::Printf(TEXT("ExtractSurfaceTriangles: Surface child not found under '%s'"), *Config.Id);
            return false;
        }

        auto* SurfaceTopo = SurfaceNode->getObject("surfaceTopo");

        /*UE_LOG(LogSofaSceneExtractor, Warning, TEXT("ExtractSurfaceTriangles: surfaceTopo => %p"),
            SurfaceTopo);*/

        if (!SurfaceTopo)
        {
            OutError = TEXT("ExtractSurfaceTriangles: surfaceTopo not found");
            return false;
        }

        auto* TrianglesData = SurfaceTopo->findData("triangles");

        /*UE_LOG(LogSofaSceneExtractor, Warning, TEXT("ExtractSurfaceTriangles: triangles data => %p"),
            TrianglesData);*/

        if (!TrianglesData)
        {
            OutError = TEXT("ExtractSurfaceTriangles: triangles data not found on surfaceTopo");
            return false;
        }

        auto* MeshTopology = dynamic_cast<sofa::core::topology::BaseMeshTopology*>(SurfaceTopo);
        /*UE_LOG(LogSofaSceneExtractor, Warning, TEXT("ExtractSurfaceTriangles: BaseMeshTopology cast => %p"),
            MeshTopology);*/

        if (!MeshTopology)
        {
            OutError = TEXT("ExtractSurfaceTriangles: surfaceTopo is not a BaseMeshTopology");
            return false;
        }

        const sofa::Size TriangleCount = MeshTopology->getNbTriangles();

        /*UE_LOG(LogSofaSceneExtractor, Warning, TEXT("ExtractSurfaceTriangles: triangle count = %d"),
            (int32)TriangleCount);*/

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

        if (OutTriangles.Num() > 0)
        {
            const FSofaDebugTriangle& T0 = OutTriangles[0];
            /*UE_LOG(LogSofaSceneExtractor, Warning,
                TEXT("ExtractSurfaceTriangles: first triangle = (%d, %d, %d)"),
                T0.A, T0.B, T0.C);*/
        }
        return true;
    }

    bool ExtractVisualSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        FSofaSurfaceMeshState& OutMesh,
        FString& OutError)
    {
        OutMesh.Source = ESofaSurfaceSource::None;
        OutMesh.Vertices.Reset();
        OutMesh.Triangles.Reset();
        OutMesh.Normals.Reset();
        OutMesh.UV0.Reset();

        auto* Root = Scene.RootNode.get();
        if (!Root)
        {
            OutError = TEXT("ExtractVisualSurfaceMesh: root node is null");
            return false;
        }

        auto* ObjectNode = Root->getChild(TCHAR_TO_UTF8(*Config.Id));
        if (!ObjectNode)
        {
            OutError = FString::Printf(
                TEXT("ExtractVisualSurfaceMesh: object node '%s' not found"),
                *Config.Id);
            return false;
        }
        sofa::simulation::Node* VisualNode = ObjectNode->getChild("Visual");
        if (!VisualNode)
        {
            VisualNode = ObjectNode->getChild("SurfaceVisual");
        }
        if (!VisualNode)
        {
            OutError = FString::Printf(
                TEXT("ExtractVisualSurfaceMesh: no visual child node found under '%s'"),
                *Config.Id);
            return false;
        }

        sofa::core::objectmodel::BaseObject* BaseMState = VisualNode->getObject("visualDofs");
        if (!BaseMState)
        {
            BaseMState = VisualNode->getObject("mstate");
        }
        if (!BaseMState)
        {
            OutError = TEXT("ExtractVisualSurfaceMesh: missing 'visualDofs' or 'mstate' in visual node");
            return false;
        }

        auto* MState =
            dynamic_cast<sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>*>(BaseMState);
        if (!MState)
        {
            OutError = TEXT("ExtractVisualSurfaceMesh: failed to cast visual MechanicalObject<Vec3d>");
            return false;
        }

        sofa::core::objectmodel::BaseObject* TopoObj = VisualNode->getObject("visualTopo");
        if (!TopoObj)
        {
            TopoObj = VisualNode->getObject("topo");
        }
        if (!TopoObj)
        {
            OutError = TEXT("ExtractVisualSurfaceMesh: missing 'visualTopo' or 'topo' in visual node");
            return false;
        }

        auto* MeshTopology = dynamic_cast<sofa::core::topology::BaseMeshTopology*>(TopoObj);
        if (!MeshTopology)
        {
            OutError = TEXT("ExtractVisualSurfaceMesh: topology cast to BaseMeshTopology failed");
            return false;
        }

        const auto& Positions = MState->readPositions();
        OutMesh.Vertices.Reserve((int32)Positions.size());

        for (int32 i = 0; i < (int32)Positions.size(); ++i)
        {
            FVector SofaPos(Positions[i][0], Positions[i][1], Positions[i][2]);
            OutMesh.Vertices.Add(SofaCoordinateSystem::SofaToUnrealPosition(SofaPos, Config));
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
            OutError = TEXT("ExtractVisualSurfaceMesh: no vertices extracted");
            return false;
        }

        if (OutMesh.Triangles.Num() == 0)
        {
            OutError = TEXT("ExtractVisualSurfaceMesh: no triangles extracted");
            return false;
        }

        OutMesh.Source = ESofaSurfaceSource::VisualSurface;
        return true;
    }

    bool ExtractRenderableSurfaceMesh(
        const FSofaRuntimeScene& Scene,
        const FSofaDeformableObjectConfig& Config,
        FSofaObjectState& OutState,
        FString& OutError)
    {
        OutState.SurfaceMesh.Source = ESofaSurfaceSource::None;
        OutState.SurfaceMesh.Vertices.Reset();
        OutState.SurfaceMesh.Triangles.Reset();
        OutState.SurfaceMesh.Normals.Reset();
        OutState.SurfaceMesh.UV0.Reset();

        FString VisualError;
        FString FallbackError;

        //Extract visual debug points and triangles first. They will be used as a fallback if visual extraction fails.
        if (!ExtractMechanicalDebugPoints(Scene, Config, OutState.DebugPoints, FallbackError))
        {
            OutError = FString::Printf(
                TEXT("ExtractRenderableSurfaceMesh: visual extraction failed (%s), fallback vertices failed (%s)"),
                *VisualError,
                *FallbackError);
            return false;
        }

        if (!ExtractMechanicalSurfaceDebugTriangles(Scene, Config, OutState.SurfaceTriangles, FallbackError))
        {
            OutError = FString::Printf(
                TEXT("ExtractRenderableSurfaceMesh: visual extraction failed (%s), fallback triangles failed (%s)"),
                *VisualError,
                *FallbackError);
            return false;
        }

        // Attempt to extract the visual surface mesh first. If it fails, we will use the debug points and triangles as a fallback.
        if (Config.VisualConfig.bUseVisualMesh && ExtractVisualSurfaceMesh(Scene, Config, OutState.SurfaceMesh, VisualError))
        {
            OutState.SurfaceMesh.Source = ESofaSurfaceSource::VisualSurface;
            return true;
        }

        if (OutState.DebugPoints.Num() == 0)
        {
            OutError = FString::Printf(
                TEXT("ExtractRenderableSurfaceMesh: fallback produced no vertices (visual error: %s)"),
                *VisualError);
            return false;
        }

        if (OutState.SurfaceTriangles.Num() == 0)
        {
            OutError = FString::Printf(
                TEXT("ExtractRenderableSurfaceMesh: fallback produced no triangles (visual error: %s)"),
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