#include "SofaSceneBuilder.h"
#include "SofaSimulationService.h"
#include "Logging/LogMacros.h"

#include "SofaIncludes.h"
#include "SofaRuntimeScene.h"
#include "SofaUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaSceneBuilder, Log, All);

namespace
{
#if SOFA_SDK_ENABLED

    using sofa::core::objectmodel::BaseData;
    using sofa::core::objectmodel::BaseObject;
    using sofa::simulation::Node;
    using sofa::simulation::NodeSPtr;

    /*static bool SetDataString(BaseObject* Object, const char* DataName, const std::string& Value, FString& OutError)
    {
        if (!Object)
        {
            OutError = TEXT("SetDataString: Object is null");
            return false;
        }

        BaseData* Data = Object->findData(DataName);
        if (!Data)
        {
            const FString DataNameStr = UTF8_TO_TCHAR(DataName);
            const FString ClassNameStr = UTF8_TO_TCHAR(Object->getClassName().c_str());

            OutError = FString::Printf(
                TEXT("Missing data '%s' on component '%s'"),
                *DataNameStr,
                *ClassNameStr);
            return false;
        }

        if (!Data->read(Value))
        {
            const FString DataNameStr = UTF8_TO_TCHAR(DataName);
            const FString ValueStr = UTF8_TO_TCHAR(Value.c_str());
            const FString ClassNameStr = UTF8_TO_TCHAR(Object->getClassName().c_str());

            OutError = FString::Printf(
                TEXT("Failed to set data '%s'='%s' on component '%s'"),
                *DataNameStr,
                *ValueStr,
                *ClassNameStr);
            return false;
        }

        return true;
    }*/

    /*static BaseObject::SPtr CreateObjectByClassName(
        Node* OwnerNode,
        const char* SofaClassName,
        const char* ObjectName,
        FString& OutError)
    {
        if (!OwnerNode)
        {
            OutError = TEXT("CreateObjectByClassName: OwnerNode is null");
            return nullptr;
        }

        sofa::core::objectmodel::BaseObjectDescription Desc;
        Desc.setAttribute("type", SofaClassName);

        if (ObjectName && *ObjectName)
        {
            Desc.setAttribute("name", ObjectName);
        }

        BaseObject::SPtr Object =
            sofa::core::ObjectFactory::getInstance()->createObject(OwnerNode, &Desc);

        if (!Object)
        {
            const FString ClassNameStr = UTF8_TO_TCHAR(SofaClassName);

            OutError = FString::Printf(
                TEXT("ObjectFactory failed for SOFA class '%s'"),
                *ClassNameStr);
            return nullptr;
        }

        return Object;
    }*/

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
