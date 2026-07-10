#include "SofaSceneBuilder.h"
#include "SofaSimulationService.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "SofaRuntimeTypes.h"

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

    static FString MakeNodePath(const FString& ParentPath, const FString& NodeName)
    {
        return ParentPath.IsEmpty() ? NodeName : (ParentPath + TEXT("/") + NodeName);
    }

    static FString MakeObjectKey(const FString& NodePath, const FString& ObjectName)
    {
        return NodePath + TEXT("::") + ObjectName;
    }

    static void RegisterBuiltObject(
        FSofaRuntimeScene& Scene,
        const FString& NodePath,
        const FString& ObjectName,
        const FString& ClassName)
    {
        if (ObjectName.IsEmpty())
        {
            UE_LOG(LogSofaSceneBuilder, Warning,
                TEXT("[INDEX OBJ] Skip unnamed object | NodePath=%s Class=%s"),
                *NodePath,
                *ClassName);
            return;
        }

        FSofaIndexedObject Entry;
        Entry.NodePath = NodePath;
        Entry.ObjectName = ObjectName;
        Entry.ObjectKey = MakeObjectKey(NodePath, ObjectName);
        Entry.ClassName = ClassName;
        Entry.Generation = Scene.SceneGeneration;

        Scene.ObjectKeysByName.Add(ObjectName, Entry.ObjectKey);
        Scene.ObjectIndexByKey.Add(Entry.ObjectKey, MoveTemp(Entry));
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
        const sofa::simulation::Node::SPtr& ParentNode,
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

        FString ObjectName(Object->getName().c_str());
        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Object %s (%s) successfully added to parent %s"), *ObjectClassFString, *ObjectName, *ParentNodeName);
        return true;
    }

    bool BuildComponentOnNode(
        const sofa::simulation::Node::SPtr& Node,
        const FSofaComponentDefinition& ComponentDef,
        FString& OutError)
    {
        if (!Node)
        {
            OutError = TEXT("BuildComponentOnNode: Node is null");
            UE_LOG(LogTemp, Error,
                TEXT("[BUILD COMPONENT] Abort | Node is null | Type=%s"),
                *ComponentDef.Type);
            return false;
        }

        const FString NodeName = UTF8_TO_TCHAR(Node->getName().c_str());

        std::map<std::string, std::string> SofaParams;

        for (const TPair<FString, FString>& Pair : ComponentDef.Attributes)
        {
            SofaParams.emplace(
                TCHAR_TO_UTF8(*Pair.Key),
                TCHAR_TO_UTF8(*Pair.Value));
        }

        const bool bOk = CreateAndAddObjectToNode(
            Node,
            TCHAR_TO_UTF8(*ComponentDef.Type),
            SofaParams,
            OutError);

        return bOk;
    }

    bool BuildGlobalRootComponents(
        FSofaRuntimeScene& Scene,
        const FSofaSceneDefinition& SceneDef,
        const sofa::simulation::Node::SPtr& RootNode,
        FString& OutError)
    {
        if (!RootNode)
        {
            OutError = TEXT("BuildGlobalRootComponents: RootNode is null");
            UE_LOG(LogSofaSceneBuilder, Error, TEXT("[BUILD ROOT COMPONENTS] Abort | RootNode is null"));
            return false;
        }

        const FString RootPath = TEXT("root");

        for (const FSofaComponentDefinition& ComponentDef : SceneDef.GlobalRootComponents)
        {
            if (!BuildComponentOnNode(RootNode, ComponentDef, OutError))
            {
                UE_LOG(LogSofaSceneBuilder, Error,
                    TEXT("[BUILD ROOT COMPONENTS] Failed | Component=%s Type=%s Error=%s"),
                    *ComponentDef.Name,
                    *ComponentDef.Type,
                    *OutError);
                return false;
            }

            RegisterBuiltObject(Scene, RootPath, ComponentDef.Name, ComponentDef.Type);
        }

        return true;
    }

    bool BuildNodeRecursive(
        FSofaRuntimeScene& Scene,
        const sofa::simulation::Node::SPtr& ParentNode,
        const FString& ParentPath,
        const FSofaNodeDefinition& NodeDef,
        FString& OutError)
    {
        if (!ParentNode)
        {
            OutError = TEXT("Invalid parent node");
            UE_LOG(LogTemp, Error,
                TEXT("[BUILD NODE] Abort | ParentNode is null | NodeDef=%s"),
                *NodeDef.Name);
            return false;
        }

        const FString SafeName = NodeDef.Name.IsEmpty() ? TEXT("Node") : NodeDef.Name;

        if (NodeDef.Name.IsEmpty()) {
            UE_LOG(LogSofaSceneBuilder, Warning, TEXT("Child node of %s is empty. 'Node' given as default name"), *ParentPath);
        }

        sofa::simulation::Node::SPtr ChildNode = ParentNode->createChild(TCHAR_TO_UTF8(*SafeName));

        if (!ChildNode)
        {
            OutError = FString::Printf(TEXT("Failed to create child node '%s'"), *SafeName);
            UE_LOG(LogSofaSceneBuilder, Error,
                TEXT("[BUILD NODE] Failed | %s"),
                *OutError);
            return false;
        }

        const FString NodePath = MakeNodePath(ParentPath, SafeName);

        {
            FSofaIndexedNode IndexedNode;
            IndexedNode.Path = NodePath;
            IndexedNode.Name = SafeName;
            IndexedNode.Generation = Scene.SceneGeneration;

            Scene.NodeIndexByPath.Add(NodePath, MoveTemp(IndexedNode));
        }

        UE_LOG(LogSofaSceneBuilder, Log, TEXT("Node created : %s"), *NodePath);

        for (int32 ComponentIndex = 0; ComponentIndex < NodeDef.Components.Num(); ++ComponentIndex)
        {
            const FSofaComponentDefinition& ComponentDef = NodeDef.Components[ComponentIndex];

            sofa::core::objectmodel::BaseObject::SPtr BuiltObject;

            if (!BuildComponentOnNode(ChildNode, ComponentDef, OutError))
            {
                UE_LOG(LogSofaSceneBuilder, Error,
                    TEXT("[BUILD NODE] Component failed | Node=%s Component=%s Error=%s"),
                    *NodePath,
                    *ComponentDef.Name,
                    *OutError);
                return false;
            }

            RegisterBuiltObject(Scene, NodePath, ComponentDef.Name, ComponentDef.Type);
        }

        for (int32 ChildDefIndex = 0; ChildDefIndex < NodeDef.Children.Num(); ++ChildDefIndex)
        {
            const FSofaNodeDefinition& ChildDef = NodeDef.Children[ChildDefIndex];

            if (!BuildNodeRecursive(Scene, ChildNode, NodePath, ChildDef, OutError))
            {
                return false;
            }
        }

        return true;
    }


    static FString FindNodeRefName(
        const TArray<FSofaNodeRef>& NodeRefs,
        ESofaNodeRefRole Role)
    {
        for (const FSofaNodeRef& Ref : NodeRefs)
        {
            if (Ref.Role == Role && !Ref.Name.IsNone())
            {
                return Ref.Name.ToString();
            }
        }
        return FString();
    }

    static const FSofaObjectIntegrationOverride* FindObjectOverride(
        const FSofaSceneIntegrationOverrides& Overrides,
        const FString& ObjectId)
    {
        UE_LOG(LogTemp, Log, TEXT("Checking overrides for node %s"), *ObjectId);
        for (const FSofaObjectIntegrationOverride& It : Overrides.Objects)
        {
            UE_LOG(LogTemp, Log, TEXT("Override ID : %s, Node id : %s"), *It.ObjectId, *ObjectId);
            if (It.ObjectId == ObjectId)
            {
                return &It;
            }
        }
        return nullptr;
    }

    static const FSofaToolIntegrationOverride* FindToolOverride(
        const FSofaSceneIntegrationOverrides& Overrides,
        const FString& ToolId)
    {
        for (const FSofaToolIntegrationOverride& It : Overrides.Tools)
        {
            if (It.ToolId == ToolId)
            {
                return &It;
            }
        }
        return nullptr;
    }

    static FString FindFirstComponentNameByType(
        const FSofaNodeDefinition& NodeDef,
        const FString& ComponentType)
    {
        for (const FSofaComponentDefinition& CompDef : NodeDef.Components)
        {
            if (CompDef.Type == ComponentType)
            {
                return CompDef.Name;
            }
        }
        return FString();
    }

    static FString FindFirstTopologyContainerName(
        const FSofaNodeDefinition& NodeDef)
    {
        static const TArray<FString> TopologyTypes =
        {
            TEXT("TetrahedronSetTopologyContainer"),
            TEXT("HexahedronSetTopologyContainer"),
            TEXT("TriangleSetTopologyContainer"),
            TEXT("QuadSetTopologyContainer"),
            TEXT("EdgeSetTopologyContainer")
        };

        for (const FString& TypeName : TopologyTypes)
        {
            const FString Found = FindFirstComponentNameByType(NodeDef, TypeName);
            if (!Found.IsEmpty())
            {
                return Found;
            }
        }

        return FString();
    }

    static const FSofaNodeDefinition* FindChildNodeByName(
        const FSofaNodeDefinition& NodeDef,
        const FString& ChildName)
    {
        for (const FSofaNodeDefinition& Child : NodeDef.Children)
        {
            if (Child.Name == ChildName)
            {
                return &Child;
            }
        }
        return nullptr;
    }

    static bool IsObjectRootNodeName(const FString& Name)
    {
        return Name.StartsWith(TEXT("OBJ_"), ESearchCase::CaseSensitive);
    }

    static bool IsToolRootNodeName(const FString& Name)
    {
        return Name.StartsWith(TEXT("TOOL_"), ESearchCase::CaseSensitive);
    }

    static bool IsRuntimeToolNode(const FSofaNodeDefinition& NodeDef)
    {
        return IsToolRootNodeName(NodeDef.Name);
    }

    static bool IsRuntimeObjectNode(const FSofaNodeDefinition& NodeDef)
    {
        return IsObjectRootNodeName(NodeDef.Name);
    }

    static ESofaRuntimeObjectRole InferRuntimeObjectRole(
        const FSofaNodeDefinition& NodeDef)
    {
        const FString LowerName = NodeDef.Name.ToLower();

        const bool bHasMechanical =
            !FindFirstComponentNameByType(NodeDef, TEXT("MechanicalObject")).IsEmpty();

        const bool bHasVolumeTopology =
            !FindFirstComponentNameByType(NodeDef, TEXT("TetrahedronSetTopologyContainer")).IsEmpty() ||
            !FindFirstComponentNameByType(NodeDef, TEXT("HexahedronSetTopologyContainer")).IsEmpty();

        const bool bHasSurfaceChild =
            FindChildNodeByName(NodeDef, TEXT("Surface")) != nullptr ||
            FindChildNodeByName(NodeDef, TEXT("Collision")) != nullptr;

        const bool bHasVisualChild =
            FindChildNodeByName(NodeDef, TEXT("Visual")) != nullptr;

        if (LowerName.Contains(TEXT("tool")) ||
            LowerName.Contains(TEXT("instrument")) ||
            LowerName.Contains(TEXT("scalpel")) ||
            LowerName.Contains(TEXT("needle")) ||
            LowerName.Contains(TEXT("grasper")) ||
            LowerName.Contains(TEXT("forceps")))
        {
            return ESofaRuntimeObjectRole::Tool;
        }

        if (!bHasMechanical && bHasSurfaceChild && !bHasVisualChild)
        {
            return ESofaRuntimeObjectRole::CollisionProxy;
        }

        if (bHasMechanical && bHasVolumeTopology)
        {
            if (LowerName.Contains(TEXT("skin")) ||
                LowerName.Contains(TEXT("layer")) ||
                LowerName.Contains(TEXT("fat")) ||
                LowerName.Contains(TEXT("fascia")))
            {
                return ESofaRuntimeObjectRole::DeformableLayer;
            }

            return ESofaRuntimeObjectRole::Organ;
        }

        if (!bHasMechanical)
        {
            return ESofaRuntimeObjectRole::StaticSupport;
        }

        return ESofaRuntimeObjectRole::Unknown;
    }

    static FSofaRuntimeObjectDescriptor MakeRuntimeObjectDescriptor(
        const FSofaNodeDefinition& NodeDef,
        const FSofaSceneIntegrationOverrides& IntegrationOverrides)
    {
        FSofaRuntimeObjectDescriptor RuntimeObject;

        RuntimeObject.ObjectNodeName = NodeDef.Name;
        RuntimeObject.MechanicalObjectName = FindFirstComponentNameByType(NodeDef, TEXT("MechanicalObject"));
        RuntimeObject.TopologyContainerName = FindFirstTopologyContainerName(NodeDef);

        

        const FSofaNodeDefinition* SurfaceNode = FindChildNodeByName(NodeDef, TEXT("Surface"));
        const FSofaNodeDefinition* VisualNode = FindChildNodeByName(NodeDef, TEXT("Visual"));
        const FSofaNodeDefinition* CollisionNode = FindChildNodeByName(NodeDef, TEXT("Collision"));

        

        if (const FSofaObjectIntegrationOverride* Override = FindObjectOverride(IntegrationOverrides, RuntimeObject.ObjectNodeName))
        {
            RuntimeObject.VisualMaterialPath = Override->VisualMaterialPath;
            RuntimeObject.SofaScale = Override->SofaScale;
            RuntimeObject.UnrealAnchorTransform = FTransform(Override->UnrealRotation, Override->UnrealTranslation, FVector::OneVector);
            RuntimeObject.bPreferVisualSurface = Override->bPreferVisualSurface;
            RuntimeObject.Role = Override->Role;

            const FString SurfaceNodeNameFromOverride = FindNodeRefName(Override->NodeRefs, ESofaNodeRefRole::Surface);
            if (!SurfaceNodeNameFromOverride.IsEmpty())
            {
                if (const FSofaNodeDefinition* OverrideSurfaceNode =
                    FindChildNodeByName(NodeDef, SurfaceNodeNameFromOverride))
                {
                    SurfaceNode = OverrideSurfaceNode;
                }
            }
            const FString VisualNodeNameFromOverride = FindNodeRefName(Override->NodeRefs, ESofaNodeRefRole::Visual);
            if (!VisualNodeNameFromOverride.IsEmpty())
            {
                if (const FSofaNodeDefinition* OverrideVisualNode =
                    FindChildNodeByName(NodeDef, VisualNodeNameFromOverride))
                {
                    VisualNode = OverrideVisualNode;
                }
            }
            const FString CollisionNodeNameFromOverride = FindNodeRefName(Override->NodeRefs, ESofaNodeRefRole::Collision);
            if (!CollisionNodeNameFromOverride.IsEmpty())
            {
                if (const FSofaNodeDefinition* OverrideCollisionNode =
                    FindChildNodeByName(NodeDef, CollisionNodeNameFromOverride))
                {
                    CollisionNode = OverrideCollisionNode;
                }
            }
        }
        else
        {
            RuntimeObject.VisualMaterialPath.Reset();
            RuntimeObject.SofaScale = 10.0f;
            RuntimeObject.UnrealAnchorTransform = FTransform::Identity;
            RuntimeObject.bPreferVisualSurface = true;
        }
        if (SurfaceNode)
        {
            RuntimeObject.SurfaceNodeName = SurfaceNode->Name;
            RuntimeObject.SurfaceTopologyName = FindFirstTopologyContainerName(*SurfaceNode);
        }

        if (VisualNode)
        {
            RuntimeObject.VisualNodeName = VisualNode->Name;
            RuntimeObject.VisualMechanicalObjectName = FindFirstComponentNameByType(*VisualNode, TEXT("MechanicalObject"));
            RuntimeObject.VisualTopologyName = FindFirstTopologyContainerName(*VisualNode);
        }

        return RuntimeObject;
    }

    static FSofaRuntimeToolDescriptor MakeRuntimeToolDescriptor(
        const FSofaNodeDefinition& NodeDef,
        const FSofaSceneIntegrationOverrides& IntegrationOverrides)
    {
        FSofaRuntimeToolDescriptor RuntimeTool;

        RuntimeTool.ToolNodeName = FName(NodeDef.Name);
        RuntimeTool.ControlMechanicalObjectName = FName(FindFirstComponentNameByType(NodeDef, TEXT("MechanicalObject")));

        const FSofaNodeDefinition* CollisionNode = FindChildNodeByName(NodeDef, TEXT("PrimaryToolCollision"));

        if (const FSofaToolIntegrationOverride* Override = FindToolOverride(IntegrationOverrides, NodeDef.Name))
        {
            const FString CollisionNodeNameFromOverride = FindNodeRefName(Override->NodeRefs, ESofaNodeRefRole::Collision);
            if (!CollisionNodeNameFromOverride.IsEmpty())
            {
                if (const FSofaNodeDefinition* OverrideCollisionNode =
                    FindChildNodeByName(NodeDef, CollisionNodeNameFromOverride))
                {
                    CollisionNode = OverrideCollisionNode;
                }
            }
            RuntimeTool.UnrealAnchorTransform = FTransform(Override->UnrealRotation, Override->UnrealTranslation, FVector::OneVector);
            RuntimeTool.SofaScale = FMath::IsNearlyZero(Override->SofaScale) ? 10.0f : Override->SofaScale;
            RuntimeTool.bVisible = Override->bVisible;
        }
        else {
            RuntimeTool.UnrealAnchorTransform = FTransform::Identity;
            RuntimeTool.SofaScale = 10.0f;
            RuntimeTool.bVisible = true;
        }
        return RuntimeTool;
    }

    static void CollectRuntimeObjectsFromNodeRecursive(
        const FSofaNodeDefinition& NodeDef,
        const FSofaSceneIntegrationOverrides& IntegrationOverrides,
        TArray<FSofaRuntimeObjectDescriptor>& OutRuntimeObjects)
    {
        if (IsRuntimeObjectNode(NodeDef))
        {
            OutRuntimeObjects.Add(MakeRuntimeObjectDescriptor(NodeDef, IntegrationOverrides));
        }

        for (const FSofaNodeDefinition& ChildNode : NodeDef.Children)
        {
            CollectRuntimeObjectsFromNodeRecursive(ChildNode, IntegrationOverrides, OutRuntimeObjects);
        }
    }

    static void CollectRuntimeToolsFromNodeRecursive(
        const FSofaNodeDefinition& NodeDef,
        const FSofaSceneIntegrationOverrides& IntegrationOverrides,
        TArray<FSofaRuntimeToolDescriptor>& OutRuntimeTools)
    {
        if (IsRuntimeToolNode(NodeDef))
        {
            OutRuntimeTools.Add(MakeRuntimeToolDescriptor(NodeDef, IntegrationOverrides));
        }
        for (const FSofaNodeDefinition& ChildNode : NodeDef.Children)
        {
            CollectRuntimeToolsFromNodeRecursive(ChildNode, IntegrationOverrides, OutRuntimeTools);
        }
    }

    static void DumpNodeTreeFromIndex(const FSofaRuntimeScene& Scene)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[DUMP TREE] Begin indexed dump | SceneGeneration=%llu IndexedNodeCount=%d"),
            Scene.SceneGeneration,
            Scene.NodeIndexByPath.Num());

        if (Scene.NodeIndexByPath.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[DUMP TREE] <empty node index>"));
            return;
        }

        TArray<const FSofaIndexedNode*> IndexedNodes;
        IndexedNodes.Reserve(Scene.NodeIndexByPath.Num());

        for (const TPair<FString, FSofaIndexedNode>& Pair : Scene.NodeIndexByPath)
        {
            const FSofaIndexedNode& IndexedNode = Pair.Value;

            if (IndexedNode.Generation != Scene.SceneGeneration)
            {
                continue;
            }

            IndexedNodes.Add(&IndexedNode);
        }

        IndexedNodes.Sort([](const FSofaIndexedNode& A, const FSofaIndexedNode& B)
            {
                return A.Path < B.Path;
            });

        for (const FSofaIndexedNode* IndexedNode : IndexedNodes)
        {
            if (!IndexedNode)
            {
                continue;
            }

            FString Path = IndexedNode->Path;
            TArray<FString> Segments;
            Path.ParseIntoArray(Segments, TEXT("/"), true);

            const int32 Depth = FMath::Max(0, Segments.Num() - 1);
            const FString Indent = FString::ChrN(Depth * 2, TCHAR(' '));

            FString ParentPath;

            if (Segments.Num() > 1)
            {
                TArray<FString> ParentSegments = Segments;
                ParentSegments.Pop(); // retire le dernier segment
                ParentPath = FString::Join(ParentSegments, TEXT("/"));
            }

            int32 ChildCount = 0;
            for (const TPair<FString, FSofaIndexedNode>& OtherPair : Scene.NodeIndexByPath)
            {
                const FSofaIndexedNode& OtherNode = OtherPair.Value;

                if (OtherNode.Generation != Scene.SceneGeneration)
                {
                    continue;
                }

                if (OtherNode.Path == IndexedNode->Path)
                {
                    continue;
                }

                TArray<FString> OtherSegments;
                OtherNode.Path.ParseIntoArray(OtherSegments, TEXT("/"), true);

                if (OtherSegments.Num() != Segments.Num() + 1)
                {
                    continue;
                }

                FString OtherParentPath;

                if (OtherSegments.Num() > 1)
                {
                    const int32 LastSlashIndex = OtherNode.Path.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

                    if (LastSlashIndex != INDEX_NONE)
                    {
                        OtherParentPath = OtherNode.Path.Left(LastSlashIndex);
                    }
                }

                if (OtherParentPath == IndexedNode->Path)
                {
                    ++ChildCount;
                }
            }

            UE_LOG(LogTemp, Warning,
                TEXT("[DUMP TREE] %sNode='%s' Path='%s' Depth=%d Parent='%s' ChildCount=%d"),
                *Indent,
                *IndexedNode->Name,
                *IndexedNode->Path,
                Depth,
                ParentPath.IsEmpty() ? TEXT("<none>") : *ParentPath,
                ChildCount);

            for (const TPair<FString, FSofaIndexedObject>& ObjectPair : Scene.ObjectIndexByKey)
            {
                const FSofaIndexedObject& IndexedObject = ObjectPair.Value;

                if (IndexedObject.Generation != Scene.SceneGeneration)
                {
                    continue;
                }

                if (IndexedObject.NodePath != IndexedNode->Path)
                {
                    continue;
                }

                UE_LOG(LogTemp, Warning,
                    TEXT("[DUMP TREE] %s  Object='%s' Class='%s' Key='%s'"),
                    *Indent,
                    *IndexedObject.ObjectName,
                    *IndexedObject.ClassName,
                    *IndexedObject.ObjectKey);
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("[DUMP TREE] End indexed dump"));
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

    Root->setName("root");
    Root->setAnimate(true);

    ApplyGlobalSceneAttributes(SceneDef, Root);

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

    SofaContext.SimulationPtr = Simu;
    SofaContext.RootNode = Root;
    SofaContext.LoadedScenePath = SceneDef.SourceFilePath;
    SofaContext.SceneName = Request.bUseSceneFilePath ? FPaths::GetBaseFilename(Request.SceneFilePath) : Request.SceneName;
    ++SofaContext.SceneGeneration;

    SofaContext.RuntimeObjects.Reset();
    SofaContext.RuntimeTools.Reset();
    SofaContext.Bindings.Reset();
    SofaContext.NodeIndexByPath.Reset();
    SofaContext.ObjectIndexByKey.Reset();
    SofaContext.ObjectKeysByName.Reset();

    {
        FSofaIndexedNode RootEntry;
        RootEntry.Path = TEXT("root");
        RootEntry.Name = TEXT("root");
        RootEntry.Generation = SofaContext.SceneGeneration;
        SofaContext.NodeIndexByPath.Add(RootEntry.Path, MoveTemp(RootEntry));
    }

    {
        FString RootComponentsError;
        if (!BuildGlobalRootComponents(SofaContext, SceneDef, Root, RootComponentsError))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to build root-level components: %s"),
                *RootComponentsError);
            return Result;
        }
    }

    {
        FString BuildError;
        const FString RootPath = TEXT("root");

        for (const FSofaNodeDefinition& ChildNodeDef : SceneDef.RootNode.Children)
        {
            if (!BuildNodeRecursive(SofaContext, Root, RootPath, ChildNodeDef, BuildError))
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Failed to build top-level simulation node '%s': %s"),
                    *ChildNodeDef.Name,
                    *BuildError);
                return Result;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[SOFA TREE] ===== BEGIN DUMP ====="));
    DumpNodeTreeFromIndex(SofaContext);
    UE_LOG(LogTemp, Warning, TEXT("[SOFA TREE] ===== END DUMP ====="));

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

    CollectRuntimeObjectsFromNodeRecursive(SceneDef.RootNode, IntegrationOverrides, SofaContext.RuntimeObjects);
    CollectRuntimeToolsFromNodeRecursive(SceneDef.RootNode, IntegrationOverrides, SofaContext.RuntimeTools);

    Result.bSuccess = true;
    return Result;
#endif
}
