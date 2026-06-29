#include "SofaSceneBuilder.h"
#include "SofaSimulationService.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"

#include "SofaIncludes.h"
#include "SofaRuntimeScene.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaSceneBuilder, Log, All);

namespace
{
#if SOFA_SDK_ENABLED

    using sofa::core::objectmodel::BaseData;
    using sofa::core::objectmodel::BaseObject;
    using sofa::simulation::Node;
    using sofa::simulation::NodeSPtr;

    bool TryParseDouble(const FString& InValue, double& OutValue)
    {
        FString Trimmed = InValue;
        Trimmed.TrimStartAndEndInline();

        if (Trimmed.IsEmpty())
        {
            return false;
        }

        OutValue = FCString::Atod(*Trimmed);
        return true;
    }

    bool TryParseVec3(
        const FString& InValue,
        sofa::type::Vec3& OutVec3)
    {
        TArray<FString> Parts;
        InValue.ParseIntoArrayWS(Parts);

        if (Parts.Num() != 3)
        {
            return false;
        }

        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;

        if (!TryParseDouble(Parts[0], X) ||
            !TryParseDouble(Parts[1], Y) ||
            !TryParseDouble(Parts[2], Z))
        {
            return false;
        }

        OutVec3 = sofa::type::Vec3(X, Y, Z);
        return true;
    }

    void ApplyGlobalSceneAttributes(
        const FSofaSceneDefinition& SceneDef,
        const sofa::simulation::NodeSPtr& RootNode)
    {
        if (!RootNode)
        {
            return;
        }

        const FString* RootName = SceneDef.GlobalRootAttributes.Find(TEXT("name"));
        if (RootName && !RootName->IsEmpty())
        {
            RootNode->setName(TCHAR_TO_UTF8(**RootName));
        }

        const FString* DtValue = SceneDef.GlobalRootAttributes.Find(TEXT("dt"));
        if (DtValue)
        {
            double ParsedDt = 0.0;
            if (TryParseDouble(*DtValue, ParsedDt))
            {
                RootNode->setDt(ParsedDt);
            }
        }

        const FString* GravityValue = SceneDef.GlobalRootAttributes.Find(TEXT("gravity"));
        if (GravityValue)
        {
            sofa::type::Vec3 ParsedGravity;
            if (TryParseVec3(*GravityValue, ParsedGravity))
            {
                RootNode->setGravity(ParsedGravity);
            }
        }
    }

    static bool AddObjectToNode(
        const NodeSPtr& OwnerNode,
        const BaseObject::SPtr& Object,
        FString& OutError)
    {
        if (!OwnerNode || !Object)
        {
            OutError = TEXT("AddObjectToNode: invalid node or object");
            return false;
        }

        OwnerNode->addObject(Object);
        return true;
    }

    static bool CreateAndAddObjectToNode(
        const sofa::simulation::NodeSPtr& ParentNode,
        const std::string& ObjectClass,
        const std::map<std::string, std::string>& Params,
        FString& OutError)
    {
        if (!ParentNode)
        {
            OutError = TEXT("Invalid parent node");
            return false;
        }

        BaseObject::SPtr Object = sofa::simpleapi::createObject(ParentNode, ObjectClass, Params);

        FString ObjectClassFString(ObjectClass.c_str());
        FString ParentNodeName(ParentNode->getName().c_str());

        if (!Object)
        {
            OutError = FString::Printf(TEXT("Failed to create object '%s' in parent node '%s'"), *ObjectClassFString, *ParentNodeName);
            return false;
        }
        if (!AddObjectToNode(ParentNode, Object, OutError))
        {
            return false;
        }

        FString ObjectName(Object->getName().c_str());
        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Object %s (%s) successfully added to parent %s"), *ObjectClassFString, *ObjectName, *ParentNodeName);
        return true;
    }

    bool BuildComponentOnNode(
        const sofa::simulation::NodeSPtr& Node,
        const FSofaComponentDefinition& ComponentDef,
        FString& OutError)
    {
        std::map<std::string, std::string> SofaParams;

        for (const TPair<FString, FString>& Pair : ComponentDef.Attributes)
        {
            SofaParams.emplace(
                TCHAR_TO_UTF8(*Pair.Key),
                TCHAR_TO_UTF8(*Pair.Value));
        }

        return CreateAndAddObjectToNode(
            Node,
            TCHAR_TO_UTF8(*ComponentDef.Type),
            SofaParams,
            OutError);
    }

    bool BuildGlobalRootComponents(
        const FSofaSceneDefinition& SceneDef,
        const sofa::simulation::NodeSPtr& RootNode,
        FString& OutError)
    {
        for (const FSofaComponentDefinition& ComponentDef : SceneDef.GlobalRootComponents)
        {
            if (!BuildComponentOnNode(RootNode, ComponentDef, OutError))
            {
                return false;
            }
        }

        return true;
    }

    bool BuildNodeRecursive(
        const sofa::simulation::NodeSPtr& ParentNode,
        const FSofaNodeDefinition& NodeDef,
        sofa::simulation::NodeSPtr& OutNode,
        FString& OutError)
    {
        if (!ParentNode)
        {
            OutError = TEXT("Invalid parent node");
            return false;
        }

        const FString SafeName = NodeDef.Name.IsEmpty() ? TEXT("Node") : NodeDef.Name;
        OutNode = ParentNode->createChild(TCHAR_TO_UTF8(*SafeName));

        if (!OutNode)
        {
            OutError = FString::Printf(TEXT("Failed to create child node '%s'"), *SafeName);
            return false;
        }

        for (const FSofaComponentDefinition& ComponentDef : NodeDef.Components)
        {
            if (!BuildComponentOnNode(OutNode, ComponentDef, OutError))
            {
                return false;
            }
        }

        for (const FSofaNodeDefinition& ChildDef : NodeDef.Children)
        {
            sofa::simulation::NodeSPtr ChildNode;
            if (!BuildNodeRecursive(OutNode, ChildDef, ChildNode, OutError))
            {
                return false;
            }
        }

        return true;
    }

    static bool AddDeformableObject(
        const NodeSPtr& ParentNode,
        const FSofaDeformableObjectConfig& ObjectConfig,
        NodeSPtr& DeformableObjectNode,
        FString& OutError)
    {
        if (!ParentNode)
        {
            OutError = TEXT("Invalid parent node");
            return false;
        }

        if (ObjectConfig.VolumeMeshPath.IsEmpty())
        {
            OutError = FString::Printf(
                TEXT("Object '%s' has empty VolumeMeshPath"),
                *ObjectConfig.Id);
            return false;
        }

        const FString SafeId = ObjectConfig.Id.IsEmpty()
            ? TEXT("SoftBody_01")
            : ObjectConfig.Id;

        DeformableObjectNode = ParentNode->createChild(TCHAR_TO_UTF8(*SafeId));
        if (!DeformableObjectNode)
        {
            OutError = FString::Printf(
                TEXT("Failed to create SOFA child node '%s'"),
                *SafeId);
            return false;
        }

        // Add EulerImplicitSolver
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "EulerImplicitSolver", { {"name", "odesolver"} }, OutError)) return false;

        // Add Linear Solver
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "CGLinearSolver", { {"name", "linear solver"}, {"iterations", "25"}, {"threshold", "1e-9"}, {"tolerance", "1e-9"} }, OutError)) return false;

        // Add MeshGmshLoader
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "MeshGmshLoader", { {"name", "loader"}, {"filename", TCHAR_TO_UTF8(*ObjectConfig.VolumeMeshPath)} }, OutError)) return false;

        // Add MechanicalObject
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "MechanicalObject", { {"name","mstate"}, {"template","Vec3d"}, {"src","@loader"} }, OutError)) return false;

        // Add TetrahedronSetTopologyContainer
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "TetrahedronSetTopologyContainer", { {"name", "topo"}, {"src", "@loader"} }, OutError)) return false;

        // Add TetrahedronSetTopologyModifier
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "TetrahedronSetTopologyModifier", { {"name","topoMod"} }, OutError)) return false;

        // Add TetrahedronSetGeometryAlgorithms
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "TetrahedronSetGeometryAlgorithms", { {"name", "GeomAlgo"}, {"template","Vec3d"} }, OutError)) return false;

        // Add UniformMass
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "UniformMass", { {"name","mass"}, {"totalMass", TCHAR_TO_UTF8(*LexToString(ObjectConfig.TotalMass))} }, OutError)) return false;

        // Add TetrahedronFEMForceField
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "TetrahedronFEMForceField",
            { {"name","forceField"}, {"template", "Vec3d"}, {"listening","true"}, {"youngModulus",TCHAR_TO_UTF8(*LexToString(ObjectConfig.YoungModulus))},
            {"poissonRatio",TCHAR_TO_UTF8(*LexToString(ObjectConfig.PoissonRatio))}, {"method","large"} }, OutError)) return false;

        // Add ConstantForceField
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "ConstantForceField", { {"name", "InteractorForce"}, {"indices", "100"}, {"totalForce", "0 0 0"} }, OutError)) return false;

        // Add FixedProjectiveConstraint
        if (!CreateAndAddObjectToNode(DeformableObjectNode, "FixedProjectiveConstraint", { {"name","FixedConstraint"}, {"indices","1 3 50"} }, OutError)) return false;
 
        return true;
    }

    static bool AddDerivedSufaceObject(
        const NodeSPtr& ParentNode,
        NodeSPtr& SurfaceNode,
        FString& OutError)
    {
        if (!ParentNode)
        {
            OutError = TEXT("Invalid parent node");
            return false;
        }

        SurfaceNode = ParentNode->createChild("Surface");
        if (!SurfaceNode)
        {
            OutError = TEXT("Failed to create Surface child node");
            return false;
        }

        // Add TriangleSetTopologyContainer
        if (!CreateAndAddObjectToNode(SurfaceNode, "TriangleSetTopologyContainer", { {"name", "surfaceTopo"} }, OutError)) return false;

        // Add TriangleSetTopologyModifier
        if (!CreateAndAddObjectToNode(SurfaceNode, "TriangleSetTopologyModifier", { {"name", "surfaceTopoMod"} }, OutError)) return false;

        // Add TriangleSetGeometryAlgorithms
        if (!CreateAndAddObjectToNode(SurfaceNode, "TriangleSetGeometryAlgorithms", { {"name", "surfaceGeom"}, {"template", "Vec3d"} }, OutError)) return false;

        // Add Tetra2TriangleTopologicalMapping
        if (!CreateAndAddObjectToNode(SurfaceNode, "Tetra2TriangleTopologicalMapping", { {"input", "@../topo"}, {"output", "@surfaceTopo"} }, OutError)) return false;
        
        return true;
    }

    static bool AddVisualObject(
        const NodeSPtr& ParentNode,
        const FSofaVisualObjectConfig& VisualConfig,
        NodeSPtr& VisualNode,
        FString& OutError)
    {
        if (!ParentNode)
        {
            OutError = TEXT("Invalid parent node");
            return false;
        }

        VisualNode = ParentNode->createChild("Visual");
        if (!VisualNode)
        {
            OutError = TEXT("Failed to create Visual child node");
            return false;
        }
        // Add MeshOBJLoader
        if (!CreateAndAddObjectToNode(VisualNode, "MeshOBJLoader", { {"name", "visualMeshLoader"}, {"filename", TCHAR_TO_UTF8(*VisualConfig.VisualMeshPath)}, {"handleSeams", VisualConfig.bHandleSeams ? "true" : "false"} }, OutError)) return false;
        
        // Add TriangleSetTopologyContainer
        if (!CreateAndAddObjectToNode(VisualNode, "TriangleSetTopologyContainer", { {"name", "visualTopo"}, {"src", "@visualMeshLoader"} }, OutError)) return false;

        // Add MechanicalObject
        if (!CreateAndAddObjectToNode(VisualNode, "MechanicalObject", { {"name","visualDofs"}, {"template","Vec3d"}, {"src","@visualMeshLoader"} }, OutError)) return false;
       
        //BarycentricMapping
        if (!CreateAndAddObjectToNode(VisualNode, "BarycentricMapping", { {"input", "@../mstate"}, {"output", "@visualDofs"} }, OutError)) return false;
        
        return true;
    }

    static void DebugLogObjectDataPresence(sofa::core::objectmodel::BaseObject* Obj, const char* ObjLabel)
    {
        if (!Obj)
        {
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("%S: null"), ObjLabel);
            return;
        }

        UE_LOG(LogSofaSceneBuilder, Warning, TEXT("%S class=%s"),
            ObjLabel,
            UTF8_TO_TCHAR(Obj->getClassName().c_str()));

        const char* Names[] = { "position", "tetrahedra", "tetras", "filename", "src" };
        for (const char* Name : Names)
        {
            auto* Data = Obj->findData(Name);
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("  data '%S' ptr=%p"), Name, Data);
        }
    }

    static void DebugLogLoaderData(sofa::simulation::Node* LiverNode)
    {
#if SOFA_SDK_ENABLED
        if (!LiverNode)
        {
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LoaderDebug: LiverNode null"));
            return;
        }

        auto* LoaderBase = LiverNode->getObject("loader");
        if (!LoaderBase)
        {
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LoaderDebug: loader not found"));
            return;
        }

        UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LoaderDebug: loader class=%s"),
            UTF8_TO_TCHAR(LoaderBase->getClassName().c_str()));

        auto* PosData = LoaderBase->findData("position");
        auto* TetData = LoaderBase->findData("tetrahedra");
        auto* FileData = LoaderBase->findData("filename");

        UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LoaderDebug: position data=%p tetrahedra data=%p filename data=%p"),
            PosData, TetData, FileData);

        if (FileData)
        {
            const std::string S = FileData->getValueString();
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LoaderDebug: filename='%s'"),
                UTF8_TO_TCHAR(S.c_str()));
        }

        if (PosData)
        {
            const std::string S = PosData->getValueString();
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LoaderDebug: positions chars=%d preview='%s'"),
                (int32)S.size(),
                UTF8_TO_TCHAR(S.substr(0, 200).c_str()));
        }

        if (TetData)
        {
            const std::string S = TetData->getValueString();
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LoaderDebug: tetrahedra chars=%d preview='%s'"),
                (int32)S.size(),
                UTF8_TO_TCHAR(S.substr(0, 200).c_str()));
        }
#endif
    }

    static const FSofaObjectIntegrationOverride* FindObjectOverride(
        const FSofaSceneIntegrationOverrides& Overrides,
        const FString& ObjectId)
    {
        for (const FSofaObjectIntegrationOverride& It : Overrides.Objects)
        {
            if (It.ObjectId == ObjectId)
            {
                return &It;
            }
        }
        return nullptr;
    }
#endif
}

FSofaSceneBuilder::FBuildResult FSofaSceneBuilder::BuildPrototypeScene(
    FSofaRuntimeScene& SofaContext,
    const FSofaSceneConfig& Config)
{
    FBuildResult Result;

#if !SOFA_SDK_ENABLED
    Result.ErrorMessage = TEXT("SOFA SDK disabled");
    return Result;
#else
    using sofa::simulation::Simulation;
    using sofa::simulation::NodeSPtr;

    UE_LOG(LogSofaSceneBuilder, Log, TEXT("Building SOFA scene: %s"), *Config.SceneId);

    sofa::simulation::common::init();
    sofa::simulation::graph::init();

    Simulation::SPtr Simu = sofa::simpleapi::createSimulation("DAG");
    if (!Simu)
    {
        Result.ErrorMessage = TEXT("Failed to create SOFA simulation");
        return Result;
    }

    NodeSPtr Root = sofa::simpleapi::createRootNode(Simu, "root");
    if (!Root)
    {
        Result.ErrorMessage = TEXT("Failed to create SOFA root node");
        return Result;
    }

    Root->setName("UE5Root");
    Root->setAnimate(true);
    Root->setDt(Config.TimeStep);
    Root->setGravity(sofa::type::Vec3(
        Config.Gravity.X,
        Config.Gravity.Y,
        Config.Gravity.Z));

    {
        FString PluginError;
        TArray<std::string> RequiredPlugins =
        {
            "Sofa.Component.LinearSolver.Direct",
            "Sofa.Component.LinearSolver.Iterative",
            "Sofa.Component.StateContainer",
            "Sofa.Component.Mass",
            "Sofa.Component.IO.Mesh",
            "Sofa.Component.Topology.Container.Dynamic",
            "Sofa.Component.Topology.Container.Constant",
            "Sofa.Component.SolidMechanics.FEM.Elastic",
            "Sofa.Component.ODESolver.Backward",
            "Sofa.Component.Constraint.Projective",
            "Sofa.Component.Topology.Mapping",
            "Sofa.Component.MechanicalLoad",
            "Sofa.Component.Mapping.Linear"
        };
        for (const std::string Plugin : RequiredPlugins)
        {
            bool success = sofa::simpleapi::importPlugin(Plugin);
            UE_LOG(LogSofaSceneBuilder, Log, TEXT("Plugin %hs import status : %s"), Plugin.c_str(), success ? TEXT("Success") : TEXT("Failure"));
        }
    }
    

    for (const FSofaDeformableObjectConfig& ObjectConfig : Config.Objects)
    {
        FString ObjectError;

        NodeSPtr DeformableObjectNode;
        if (!AddDeformableObject(Root, ObjectConfig, DeformableObjectNode, ObjectError))
        {
            Result.ErrorMessage = FString::Printf(TEXT("Failed to add deformable object '%s': %s"), *ObjectConfig.Id, *ObjectError);
            return Result;
        }
        NodeSPtr SurfaceNode;
        if (!AddDerivedSufaceObject(DeformableObjectNode, SurfaceNode, ObjectError))
        {
            Result.ErrorMessage = FString::Printf(TEXT("Failed to add derived surface object : %s"), *ObjectError);
            return Result;
        }
        NodeSPtr VisualNode;
        if (!AddVisualObject(DeformableObjectNode, ObjectConfig.VisualConfig, VisualNode, ObjectError))
        {
            Result.ErrorMessage = FString::Printf(TEXT("Failed to add visual object : %s"), *ObjectError);
            return Result;
        }
    }
    {
        try
        {
            sofa::simulation::node::initRoot(Root.get());
            UE_LOG(LogSofaSceneBuilder, Display, TEXT("SOFA root initialized after RequiredPlugin"));
        }
        catch (...)
        {
            Result.ErrorMessage = TEXT("Failed to initialize SOFA root after RequiredPlugin");
            return Result;
        }
    }
    Node* RootRaw = Root.get();
    Node* LiverNode = RootRaw ? RootRaw->getChild("Liver01") : nullptr;

    //DebugLogLoaderData(LiverNode);

    UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Post-initRoot debug begin"));
    UE_LOG(LogSofaSceneBuilder, Warning, TEXT("LiverNode ptr=%p"), LiverNode);

    if (LiverNode)
    {
        auto* LoaderObj = LiverNode->getObject("loader");
        auto* ContainerObj = LiverNode->getObject("topo");
        auto* MStateObj = LiverNode->getObject("mstate");

        //DebugLogObjectDataPresence(LoaderObj, "loader");
        //DebugLogObjectDataPresence(ContainerObj, "topo");
        //DebugLogObjectDataPresence(MStateObj, "mstate");

        auto* MState = dynamic_cast<sofa::component::statecontainer::MechanicalObject<sofa::defaulttype::Vec3Types>*>(MStateObj);
        if (MState)
        {
            const auto& X = MState->readPositions();
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("mstate.readPositions size=%d"), (int32)X.size());
        }
    }
    UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Post-initRoot debug end"));

    SofaContext.SimulationPtr = Simu;
    SofaContext.RootNode = Root;

    Result.bSuccess = true;
    return Result;
#endif
}

FSofaSceneBuilder::FBuildResult FSofaSceneBuilder::BuildPrototypeScene(
    FSofaRuntimeScene& SofaContext,
    const FSofaPrototypeSceneRequest& Request)
{
    FBuildResult Result;

#if !SOFA_SDK_ENABLED
    Result.ErrorMessage = TEXT("SOFA SDK disabled");
    return Result;
#else
    using sofa::simulation::Simulation;
    using sofa::simulation::NodeSPtr;

    if (Request.SceneFilePath.IsEmpty() && Request.SceneName.IsEmpty())
    {
        Result.ErrorMessage = TEXT("No scene file path or scene name provided");
        return Result;
    }

    FSofaSceneLoadOptions LoadOptions;
    LoadOptions.bResolveRelativePaths = true;
    LoadOptions.bNormalizeAttributes = true;
    LoadOptions.ExternalScenesDirectory = Request.ExternalScenesDirectory;
    LoadOptions.RelativeScenesDirectory = Request.RelativeScenesDirectory.IsEmpty()
        ? TEXT("SofaScenes")
        : Request.RelativeScenesDirectory;

    FSofaSceneLoadResult SceneLoadResult;
    if (Request.bUseSceneFilePath)
    {
        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Loading SOFA scene from explicit file: %s"), *Request.SceneFilePath);
        SceneLoadResult = USofaSceneLoader::LoadSceneFromScnFile(Request.SceneFilePath, LoadOptions);
    }
    else
    {
        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Loading SOFA scene by name: %s"), *Request.SceneName);
        SceneLoadResult = USofaSceneLoader::LoadSceneByName(Request.SceneName, LoadOptions);
    }

    if (!SceneLoadResult.bSuccess)
    {
        Result.ErrorMessage = FString::Printf(
            TEXT("Failed to load SOFA scene definition: %s"),
            *SceneLoadResult.ErrorMessage);
        return Result;
    }

    const FSofaSceneDefinition& SceneDef = SceneLoadResult.SceneDefinition;

    UE_LOG(LogSofaSceneBuilder, Log, TEXT("Building SOFA scene: %s"), *SceneDef.SourceFilePath);

    sofa::simulation::common::init();
    sofa::simulation::graph::init();

    Simulation::SPtr Simu = sofa::simpleapi::createSimulation("DAG");
    if (!Simu)
    {
        Result.ErrorMessage = TEXT("Failed to create SOFA simulation");
        return Result;
    }

    NodeSPtr Root = sofa::simpleapi::createRootNode(Simu, "root");
    if (!Root)
    {
        Result.ErrorMessage = TEXT("Failed to create SOFA root node");
        return Result;
    }

    Root->setName("UE5Root");
    Root->setAnimate(true);

    ApplyGlobalSceneAttributes(SceneDef, Root);

    if (SceneDef.GlobalRootAttributes.Contains(TEXT("name")))
    {
        const FString& RootName = SceneDef.GlobalRootAttributes[TEXT("name")];
        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Applied root name from scene: %s"), *RootName);
    }

    if (SceneDef.GlobalRootAttributes.Contains(TEXT("dt")))
    {
        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Applied dt from scene: %s"), *SceneDef.GlobalRootAttributes[TEXT("dt")]);
    }

    if (SceneDef.GlobalRootAttributes.Contains(TEXT("gravity")))
    {
        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Applied gravity from scene: %s"), *SceneDef.GlobalRootAttributes[TEXT("gravity")]);
    }

    for (const FString& PluginName : SceneDef.RequiredPlugins)
    {
        const std::string PluginStd = TCHAR_TO_UTF8(*PluginName);
        const bool bImported = sofa::simpleapi::importPlugin(PluginStd);

        UE_LOG(
            LogSofaSceneBuilder,
            Log,
            TEXT("Plugin %s import status : %s"),
            *PluginName,
            bImported ? TEXT("Success") : TEXT("Failure"));
    }

    {
        FString RootComponentsError;
        if (!BuildGlobalRootComponents(SceneDef, Root, RootComponentsError))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to build root-level components: %s"),
                *RootComponentsError);
            return Result;
        }
    }

    NodeSPtr SimulationNode;
    {
        FString BuildError;
        if (!BuildNodeRecursive(Root, SceneDef.RootNode, SimulationNode, BuildError))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to build simulation node '%s': %s"),
                *SceneDef.RootNode.Name,
                *BuildError);
            return Result;
        }
    }

    {
        try
        {
            sofa::simulation::node::initRoot(Root.get());
            UE_LOG(LogSofaSceneBuilder, Display, TEXT("SOFA root initialized after scene reconstruction"));
        }
        catch (...)
        {
            Result.ErrorMessage = TEXT("Failed to initialize SOFA root after scene reconstruction");
            return Result;
        }
    }

    Node* RootRaw = Root.get();
    Node* MainNode = RootRaw ? RootRaw->getChild(TCHAR_TO_UTF8(*SceneDef.RootNode.Name)) : nullptr;

    UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Post-initRoot debug begin"));
    UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Main simulation node name=%s ptr=%p"), *SceneDef.RootNode.Name, MainNode);

    if (MainNode)
    {
        if (auto* LoaderObj = MainNode->getObject("loader"))
        {
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Found object 'loader'"));
        }

        if (auto* TopoObj = MainNode->getObject("topo"))
        {
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Found object 'topo'"));
        }

        if (auto* MStateObj = MainNode->getObject("mstate"))
        {
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Found object 'mstate'"));
        }
    }

    UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Post-initRoot debug end"));

    FSofaSceneIntegrationOverrides IntegrationOverrides;
    {
        FString JsonError;
        const FString JsonOverridePath = FPaths::GetPath(SceneDef.SourceFilePath) / (FPaths::GetBaseFilename(SceneDef.SourceFilePath) + TEXT(".ue.json"));

        if (FPaths::FileExists(JsonOverridePath))
        {
            if (!USofaSceneLoader::LoadOverridesFromJsonFile(
                JsonOverridePath,
                IntegrationOverrides,
                JsonError))
            {
                UE_LOG(LogSofaSceneBuilder, Warning,
                    TEXT("Failed to load scene integration overrides '%s': %s"),
                    *JsonOverridePath,
                    *JsonError);
            }
            else
            {
                UE_LOG(LogSofaSceneBuilder, Log,
                    TEXT("Loaded scene integration overrides: %s"),
                    *JsonOverridePath);
            }
        }
    }

    SofaContext.SimulationPtr = Simu;

    SofaContext.RuntimeObjects.Reset();
    FSofaRuntimeObjectDescriptor RuntimeObject;
    RuntimeObject.ObjectId = SceneDef.RootNode.Name;
    RuntimeObject.SimulationNodeName = SceneDef.RootNode.Name;

    RuntimeObject.MechanicalObjectName = TEXT("mstate");
    RuntimeObject.TopologyContainerName = TEXT("topo");

    RuntimeObject.SurfaceNodeName = TEXT("Surface");
    RuntimeObject.SurfaceTopologyName = TEXT("surfaceTopo");

    RuntimeObject.VisualNodeName = TEXT("Visual");
    RuntimeObject.VisualMechanicalObjectName = TEXT("visualDofs");
    RuntimeObject.VisualTopologyName = TEXT("visualTopo");

    if (const FSofaObjectIntegrationOverride* Override = FindObjectOverride(IntegrationOverrides, RuntimeObject.ObjectId))
    {
        RuntimeObject.VisualMaterialPath = Override->VisualMaterialPath;
        RuntimeObject.SofaScale = Override->SofaScale;
        RuntimeObject.UnrealObjectTransform = FTransform(Override->UnrealRotation, Override->UnrealTranslation, FVector::OneVector);
        RuntimeObject.bPreferVisualSurface = Override->bPreferVisualSurface;
    }
    else
    {
        RuntimeObject.VisualMaterialPath.Reset();
        RuntimeObject.SofaScale = 10.0f;
        RuntimeObject.UnrealObjectTransform = FTransform::Identity;
        RuntimeObject.bPreferVisualSurface = true;
    }

    SofaContext.RuntimeObjects.Add(RuntimeObject);
    SofaContext.RootNode = Root;
    SofaContext.LoadedScenePath = SceneDef.SourceFilePath;
    SofaContext.SceneName = SceneDef.RootNode.Name;

    Result.bSuccess = true;
    return Result;
#endif
}
