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
    RuntimeObject.InteractorForceFieldName = TEXT("Liver01.mstate");

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
