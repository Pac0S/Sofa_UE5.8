#include "SofaSceneLoader.h"

#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "XmlFile.h"
#include "XmlNode.h"
#include "JsonObjectConverter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSofaSceneLoader, Log, All);

TUniquePtr<FSofaSceneDefinition> USofaSceneLoader::LastLoadedSceneDefinition = nullptr;

bool USofaSceneLoader::ProcessSceneFile(
    const FString& SceneFilePath,
    FString& OutResolvedPath,
    FString& OutErrorMessage)
{
    OutResolvedPath.Reset();
    OutErrorMessage.Reset();

    FSofaSceneLoadOptions Options;
    Options.ExternalScenesDirectory = GetDefaultExternalScenesDirectory();
    Options.RelativeScenesDirectory = GetDefaultRelativeScenesDirectory();

    FString ResolvedPath;
    if (!ResolveSceneFilePath(SceneFilePath, Options, ResolvedPath, OutErrorMessage))
    {
        LastLoadedSceneDefinition.Reset();
        return false;
    }

    const FSofaSceneLoadResult LoadResult = LoadSceneFromScnFile(ResolvedPath, Options);
    if (!LoadResult.bSuccess)
    {
        LastLoadedSceneDefinition.Reset();
        OutErrorMessage = LoadResult.ErrorMessage;
        return false;
    }

    LastLoadedSceneDefinition = MakeUnique<FSofaSceneDefinition>(LoadResult.SceneDefinition);
    OutResolvedPath = ResolvedPath;
    return true;
}

bool USofaSceneLoader::ProcessSceneByName(
    const FString& SceneFileName,
    const FString& ExternalScenesDirectory,
    const FString& RelativeScenesDirectory,
    FString& OutResolvedPath,
    FString& OutErrorMessage)
{
    OutResolvedPath.Reset();
    OutErrorMessage.Reset();

    FSofaSceneLoadOptions Options;
    Options.ExternalScenesDirectory = ExternalScenesDirectory.IsEmpty()
        ? GetDefaultExternalScenesDirectory()
        : ExternalScenesDirectory;

    Options.RelativeScenesDirectory = RelativeScenesDirectory.IsEmpty()
        ? GetDefaultRelativeScenesDirectory()
        : RelativeScenesDirectory;

    FString ResolvedPath;
    if (!ResolveNamedSceneFilePath(SceneFileName, Options, ResolvedPath, OutErrorMessage))
    {
        LastLoadedSceneDefinition.Reset();
        return false;
    }

    const FSofaSceneLoadResult LoadResult = LoadSceneFromScnFile(ResolvedPath, Options);
    if (!LoadResult.bSuccess)
    {
        LastLoadedSceneDefinition.Reset();
        OutErrorMessage = LoadResult.ErrorMessage;
        return false;
    }

    LastLoadedSceneDefinition = MakeUnique<FSofaSceneDefinition>(LoadResult.SceneDefinition);
    OutResolvedPath = ResolvedPath;
    return true;
}

FString USofaSceneLoader::GetDefaultExternalScenesDirectory()
{
    return TEXT("C:/Users/Pakito/Documents/Projets/Anisim/SofaScenes");
}

FString USofaSceneLoader::GetDefaultRelativeScenesDirectory()
{
    return TEXT("SofaScenes");
}

FString USofaSceneLoader::GetExecutableRelativeScenesDirectoryAbsolute(
    const FString& RelativeScenesDirectory)
{
    const FString BaseDir = FPlatformProcess::BaseDir();
    FString CombinedPath = FPaths::Combine(BaseDir, RelativeScenesDirectory);
    FPaths::CollapseRelativeDirectories(CombinedPath);
    FPaths::NormalizeFilename(CombinedPath);
    return CombinedPath;
}

FString USofaSceneLoader::BuildSceneFilePath(
    const FString& DirectoryPath,
    const FString& SceneFileName)
{
    FString CombinedPath = FPaths::Combine(DirectoryPath, SceneFileName);
    FPaths::CollapseRelativeDirectories(CombinedPath);
    FPaths::NormalizeFilename(CombinedPath);
    return CombinedPath;
}

bool USofaSceneLoader::FileExists(const FString& FilePath)
{
    FString NormalizedPath = FilePath;
    FPaths::NormalizeFilename(NormalizedPath);
    return FPaths::FileExists(NormalizedPath);
}

FSofaSceneLoadResult USofaSceneLoader::LoadSceneFromScnFile(
    const FString& SceneFilePath,
    const FSofaSceneLoadOptions& Options)
{
    FSofaSceneLoadResult Result;
    Result.bSuccess = false;

    FString ResolvedPath;
    FString ResolveError;
    if (!ResolveSceneFilePath(SceneFilePath, Options, ResolvedPath, ResolveError))
    {
        Result.ErrorMessage = ResolveError;
        return Result;
    }

    if (!FPaths::FileExists(ResolvedPath))
    {
        Result.ErrorMessage = FString::Printf(
            TEXT("Scene file does not exist: %s"),
            *ResolvedPath);
        return Result;
    }

    FSofaSceneDefinition SceneDefinition;
    FString ParseError;
    if (!ParseSceneDocument(ResolvedPath, Options, SceneDefinition, ParseError))
    {
        Result.ErrorMessage = ParseError;
        return Result;
    }

    Result.bSuccess = true;
    Result.SceneDefinition = MoveTemp(SceneDefinition);
    return Result;
}

FSofaSceneLoadResult USofaSceneLoader::LoadSceneByName(
    const FString& SceneFileName,
    const FSofaSceneLoadOptions& Options)
{
    FSofaSceneLoadResult Result;
    Result.bSuccess = false;

    FString ResolvedPath;
    FString ResolveError;
    if (!ResolveNamedSceneFilePath(SceneFileName, Options, ResolvedPath, ResolveError))
    {
        Result.ErrorMessage = ResolveError;
        return Result;
    }

    return LoadSceneFromScnFile(ResolvedPath, Options);
}

const FSofaSceneDefinition* USofaSceneLoader::GetLastLoadedSceneDefinition()
{
    return LastLoadedSceneDefinition.Get();
}

bool USofaSceneLoader::ResolveSceneFilePath(
    const FString& SceneFilePathOrName,
    const FSofaSceneLoadOptions& Options,
    FString& OutResolvedPath,
    FString& OutErrorMessage)
{
    OutResolvedPath.Reset();
    OutErrorMessage.Reset();

    if (SceneFilePathOrName.IsEmpty())
    {
        OutErrorMessage = TEXT("Scene file path or name is empty.");
        return false;
    }

    FString Candidate = SceneFilePathOrName;
    FPaths::NormalizeFilename(Candidate);

    const bool bLooksLikeExplicitPath =
        SceneFilePathOrName.Contains(TEXT("/")) ||
        SceneFilePathOrName.Contains(TEXT("\\")) ||
        SceneFilePathOrName.Contains(TEXT(":"));

    if (bLooksLikeExplicitPath)
    {
        if (FPaths::FileExists(Candidate))
        {
            OutResolvedPath = Candidate;
            return true;
        }

        OutErrorMessage = FString::Printf(
            TEXT("Explicit scene file path does not exist: %s"),
            *Candidate);
        return false;
    }

    return ResolveNamedSceneFilePath(SceneFilePathOrName, Options, OutResolvedPath, OutErrorMessage);
}

bool USofaSceneLoader::ResolveNamedSceneFilePath(
    const FString& SceneFileName,
    const FSofaSceneLoadOptions& Options,
    FString& OutResolvedPath,
    FString& OutErrorMessage)
{
    OutResolvedPath.Reset();
    OutErrorMessage.Reset();

    if (SceneFileName.IsEmpty())
    {
        OutErrorMessage = TEXT("Scene file name is empty.");
        return false;
    }

    FString EffectiveFileName = SceneFileName;
    if (!EffectiveFileName.EndsWith(TEXT(".scn")))
    {
        EffectiveFileName += TEXT(".scn");
    }

    if (!Options.ExternalScenesDirectory.IsEmpty())
    {
        const FString ExternalDir = NormalizeDirectoryPath(Options.ExternalScenesDirectory);
        const FString ExternalPath = BuildSceneFilePath(ExternalDir, EffectiveFileName);

        if (FPaths::FileExists(ExternalPath))
        {
            OutResolvedPath = ExternalPath;
            return true;
        }
    }

    const FString RelativeDir = Options.RelativeScenesDirectory.IsEmpty()
        ? GetDefaultRelativeScenesDirectory()
        : Options.RelativeScenesDirectory;

    const FString InstallScenesDir = GetExecutableRelativeScenesDirectoryAbsolute(RelativeDir);
    const FString RelativePath = BuildSceneFilePath(InstallScenesDir, EffectiveFileName);

    if (FPaths::FileExists(RelativePath))
    {
        OutResolvedPath = RelativePath;
        return true;
    }

    OutErrorMessage = FString::Printf(
        TEXT("Scene file '%s' not found in external directory '%s' or executable-relative directory '%s'."),
        *EffectiveFileName,
        *Options.ExternalScenesDirectory,
        *InstallScenesDir);

    return false;
}

bool USofaSceneLoader::ParseSceneDocument(
    const FString& SceneFilePath,
    const FSofaSceneLoadOptions& Options,
    FSofaSceneDefinition& OutSceneDefinition,
    FString& OutErrorMessage)
{
    FXmlFile XmlFile(SceneFilePath, EConstructMethod::ConstructFromFile);

    if (!XmlFile.IsValid())
    {
        OutErrorMessage = FString::Printf(
            TEXT("Invalid XML in scene file: %s"),
            *SceneFilePath);
        return false;
    }

    const FXmlNode* RootXmlNode = XmlFile.GetRootNode();
    if (!RootXmlNode)
    {
        OutErrorMessage = FString::Printf(
            TEXT("Missing XML root node in scene file: %s"),
            *SceneFilePath);
        return false;
    }

    if (!IsNodeElement(RootXmlNode))
    {
        OutErrorMessage = FString::Printf(
            TEXT("Root XML element is not a SOFA Node in file: %s"),
            *SceneFilePath);
        return false;
    }

    OutSceneDefinition.SourceFilePath = SceneFilePath;
    OutSceneDefinition.SceneDirectory = FPaths::GetPath(SceneFilePath);
    OutSceneDefinition.RequiredPlugins.Reset();
    OutSceneDefinition.GlobalRootAttributes.Reset();
    OutSceneDefinition.GlobalRootComponents.Reset();

    ExtractAttributes(RootXmlNode, OutSceneDefinition.GlobalRootAttributes);

    bool bFoundSimulationRoot = false;

    const TArray<FXmlNode*>& RootChildren = RootXmlNode->GetChildrenNodes();
    for (const FXmlNode* ChildNode : RootChildren)
    {
        if (!ChildNode)
        {
            continue;
        }

        if (IsPluginsNode(ChildNode))
        {
            if (!ParsePluginNode(ChildNode, OutSceneDefinition, OutErrorMessage))
            {
                return false;
            }
            continue;
        }

        if (IsNodeElement(ChildNode))
        {
            if (bFoundSimulationRoot)
            {
                UE_LOG(
                    LogSofaSceneLoader,
                    Warning,
                    TEXT("Multiple top-level simulation nodes found. Only the first one will be used: '%s'"),
                    *OutSceneDefinition.RootNode.Name);
                continue;
            }

            FSofaNodeDefinition RootNodeDefinition;
            if (!ParseNodeElement(
                ChildNode,
                OutSceneDefinition.SceneDirectory,
                Options,
                RootNodeDefinition,
                OutErrorMessage))
            {
                return false;
            }

            OutSceneDefinition.RootNode = MoveTemp(RootNodeDefinition);
            bFoundSimulationRoot = true;
            continue;
        }

        if (IsComponentElement(ChildNode))
        {
            FSofaComponentDefinition RootComponentDefinition;
            if (!ParseComponentElement(
                ChildNode,
                OutSceneDefinition.SceneDirectory,
                Options,
                RootComponentDefinition,
                OutErrorMessage))
            {
                return false;
            }

            OutSceneDefinition.GlobalRootComponents.Add(MoveTemp(RootComponentDefinition));
            continue;
        }

        UE_LOG(
            LogSofaSceneLoader,
            Verbose,
            TEXT("Skipping unsupported root-level element '%s'."),
            *ChildNode->GetTag());
    }

    if (!bFoundSimulationRoot)
    {
        OutErrorMessage = FString::Printf(
            TEXT("No top-level simulation node found in scene file: %s"),
            *SceneFilePath);
        return false;
    }

    return true;
}

bool USofaSceneLoader::ParsePluginNode(
    const FXmlNode* XmlNode,
    FSofaSceneDefinition& OutSceneDefinition,
    FString& OutErrorMessage)
{
    if (!XmlNode)
    {
        OutErrorMessage = TEXT("ParsePluginNode received a null XmlNode.");
        return false;
    }

    if (!IsPluginsNode(XmlNode))
    {
        OutErrorMessage = FString::Printf(
            TEXT("XML element '%s' is not a plugins node."),
            *XmlNode->GetTag());
        return false;
    }

    const TArray<FXmlNode*>& Children = XmlNode->GetChildrenNodes();
    for (const FXmlNode* ChildNode : Children)
    {
        if (!ChildNode)
        {
            continue;
        }

        if (!IsRequiredPluginElement(ChildNode))
        {
            UE_LOG(
                LogSofaSceneLoader,
                Verbose,
                TEXT("Skipping non-RequiredPlugin element '%s' inside plugins node."),
                *ChildNode->GetTag());
            continue;
        }

        FString PluginName = ChildNode->GetAttribute(TEXT("pluginName"));
        if (PluginName.IsEmpty())
        {
            PluginName = ChildNode->GetAttribute(TEXT("name"));
        }

        if (PluginName.IsEmpty())
        {
            UE_LOG(
                LogSofaSceneLoader,
                Warning,
                TEXT("RequiredPlugin entry without pluginName/name attribute ignored."));
            continue;
        }

        OutSceneDefinition.RequiredPlugins.Add(PluginName);
    }

    return true;
}

bool USofaSceneLoader::ParseNodeElement(
    const FXmlNode* XmlNode,
    const FString& SceneDirectory,
    const FSofaSceneLoadOptions& Options,
    FSofaNodeDefinition& OutNodeDefinition,
    FString& OutErrorMessage)
{
    if (!XmlNode)
    {
        OutErrorMessage = TEXT("ParseNodeElement received a null XmlNode.");
        return false;
    }

    if (!IsNodeElement(XmlNode))
    {
        OutErrorMessage = FString::Printf(
            TEXT("XML element '%s' is not a SOFA Node."),
            *XmlNode->GetTag());
        return false;
    }

    ExtractAttributes(XmlNode, OutNodeDefinition.Attributes);

    OutNodeDefinition.Name = XmlNode->GetAttribute(TEXT("name"));
    if (OutNodeDefinition.Name.IsEmpty())
    {
        OutNodeDefinition.Name = TEXT("Node");
    }

    OutNodeDefinition.Components.Reset();
    OutNodeDefinition.Children.Reset();

    const TArray<FXmlNode*>& Children = XmlNode->GetChildrenNodes();
    for (const FXmlNode* ChildXmlNode : Children)
    {
        if (!ChildXmlNode)
        {
            continue;
        }

        if (IsNodeElement(ChildXmlNode))
        {
            FSofaNodeDefinition ChildNodeDefinition;
            if (!ParseNodeElement(
                ChildXmlNode,
                SceneDirectory,
                Options,
                ChildNodeDefinition,
                OutErrorMessage))
            {
                return false;
            }

            OutNodeDefinition.Children.Add(MoveTemp(ChildNodeDefinition));
        }
        else if (IsComponentElement(ChildXmlNode))
        {
            FSofaComponentDefinition ComponentDefinition;
            if (!ParseComponentElement(
                ChildXmlNode,
                SceneDirectory,
                Options,
                ComponentDefinition,
                OutErrorMessage))
            {
                return false;
            }

            OutNodeDefinition.Components.Add(MoveTemp(ComponentDefinition));
        }
        else
        {
            UE_LOG(
                LogSofaSceneLoader,
                Verbose,
                TEXT("Skipping unsupported XML element '%s'."),
                *ChildXmlNode->GetTag());
        }
    }

    return true;
}

bool USofaSceneLoader::ParseComponentElement(
    const FXmlNode* XmlNode,
    const FString& SceneDirectory,
    const FSofaSceneLoadOptions& Options,
    FSofaComponentDefinition& OutComponentDefinition,
    FString& OutErrorMessage)
{
    if (!XmlNode)
    {
        OutErrorMessage = TEXT("ParseComponentElement received a null XmlNode.");
        return false;
    }

    OutComponentDefinition.Type = XmlNode->GetTag();
    OutComponentDefinition.Name = XmlNode->GetAttribute(TEXT("name"));

    ExtractAttributes(XmlNode, OutComponentDefinition.Attributes);
    NormalizeAttributes(OutComponentDefinition.Attributes, SceneDirectory, Options);

    return true;
}

void USofaSceneLoader::ExtractAttributes(
    const FXmlNode* XmlNode,
    TMap<FString, FString>& OutAttributes)
{
    OutAttributes.Reset();

    if (!XmlNode)
    {
        return;
    }

    const TArray<FXmlAttribute>& Attributes = XmlNode->GetAttributes();
    for (const FXmlAttribute& Attribute : Attributes)
    {
        OutAttributes.Add(Attribute.GetTag(), Attribute.GetValue());
    }
}

bool USofaSceneLoader::IsPluginsNode(const FXmlNode* XmlNode)
{
    if (!XmlNode || !IsNodeElement(XmlNode))
    {
        return false;
    }

    return XmlNode->GetAttribute(TEXT("name")).Equals(TEXT("plugins"), ESearchCase::IgnoreCase);
}

bool USofaSceneLoader::IsRequiredPluginElement(const FXmlNode* XmlNode)
{
    if (!XmlNode)
    {
        return false;
    }

    return XmlNode->GetTag().Equals(TEXT("RequiredPlugin"), ESearchCase::IgnoreCase);
}

bool USofaSceneLoader::IsNodeElement(const FXmlNode* XmlNode)
{
    if (!XmlNode)
    {
        return false;
    }

    return XmlNode->GetTag().Equals(TEXT("Node"), ESearchCase::IgnoreCase);
}

bool USofaSceneLoader::IsComponentElement(const FXmlNode* XmlNode)
{
    if (!XmlNode)
    {
        return false;
    }

    if (IsNodeElement(XmlNode))
    {
        return false;
    }

    const FString Tag = XmlNode->GetTag();
    if (Tag.IsEmpty())
    {
        return false;
    }

    if (Tag.Equals(TEXT("include"), ESearchCase::IgnoreCase))
    {
        return false;
    }

    if (Tag.Equals(TEXT("RequiredPlugin"), ESearchCase::IgnoreCase))
    {
        return false;
    }

    return true;
}

FString USofaSceneLoader::ResolveScenePath(
    const FString& InPath,
    const FString& SceneDirectory)
{
    if (InPath.IsEmpty())
    {
        return InPath;
    }

    if (InPath.StartsWith(TEXT("@")))
    {
        return InPath;
    }

    FString NormalizedInput = InPath;
    FPaths::NormalizeFilename(NormalizedInput);

    if (FPaths::IsRelative(NormalizedInput))
    {
        FString CombinedPath = FPaths::Combine(SceneDirectory, NormalizedInput);
        FPaths::CollapseRelativeDirectories(CombinedPath);
        FPaths::NormalizeFilename(CombinedPath);
        return CombinedPath;
    }

    return NormalizedInput;
}

FString USofaSceneLoader::NormalizeDirectoryPath(const FString& InDirectoryPath)
{
    FString Result = InDirectoryPath;
    FPaths::NormalizeFilename(Result);
    return Result;
}

void USofaSceneLoader::NormalizeAttributes(
    TMap<FString, FString>& InOutAttributes,
    const FString& SceneDirectory,
    const FSofaSceneLoadOptions& Options)
{
    if (!Options.bNormalizeAttributes)
    {
        return;
    }

    if (!Options.bResolveRelativePaths)
    {
        return;
    }

    static const TArray<FString> PathKeys =
    {
        TEXT("filename"),
        TEXT("textureFilename"),
        TEXT("materialFilename")
    };

    for (const FString& Key : PathKeys)
    {
        FString* ValuePtr = InOutAttributes.Find(Key);
        if (!ValuePtr || ValuePtr->IsEmpty())
        {
            continue;
        }

        *ValuePtr = ResolveScenePath(*ValuePtr, SceneDirectory);
    }
}

bool USofaSceneLoader::LoadOverridesFromJsonFile(
    const FString& JsonFilePath,
    FSofaSceneIntegrationOverrides& OutOverrides,
    FString& OutError)
{
    OutOverrides = FSofaSceneIntegrationOverrides();
    OutError.Reset();

    if (JsonFilePath.IsEmpty())
    {
        OutError = TEXT("JSON override path is empty");
        return false;
    }

    if (!FPaths::FileExists(JsonFilePath))
    {
        OutError = FString::Printf(TEXT("JSON override file not found: %s"), *JsonFilePath);
        return false;
    }

    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *JsonFilePath))
    {
        OutError = FString::Printf(TEXT("Failed to read JSON override file: %s"), *JsonFilePath);
        return false;
    }

    if (!FJsonObjectConverter::JsonObjectStringToUStruct(
        JsonContent,
        &OutOverrides,
        0,
        0))
    {
        OutError = FString::Printf(TEXT("Failed to parse JSON override file: %s"), *JsonFilePath);
        return false;
    }

    for (const FSofaObjectIntegrationOverride& ObjOverride : OutOverrides.Objects)
    {
        if (ObjOverride.ObjectId.IsEmpty())
        {
            OutError = FString::Printf(
                TEXT("Invalid override in '%s': objectId is empty"),
                *JsonFilePath);
            return false;
        }
    }
    return true;
}