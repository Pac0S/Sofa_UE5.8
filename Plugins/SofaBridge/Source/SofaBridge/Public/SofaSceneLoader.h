#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SofaSceneIntegrationOverrides.h"
#include "SofaSceneLoader.generated.h"

struct FSofaComponentDefinition
{
    FString Type;
    FString Name;
    TMap<FString, FString> Attributes;
};

struct FSofaNodeDefinition
{
    FString Name;
    TMap<FString, FString> Attributes;
    TArray<FSofaComponentDefinition> Components;
    TArray<FSofaNodeDefinition> Children;
};

struct FSofaSceneDefinition
{
    FString SourceFilePath;
    FString SceneDirectory;
    TMap<FString, FString> GlobalRootAttributes;
    TArray<FSofaComponentDefinition> GlobalRootComponents;
    TArray<FString> RequiredPlugins;
    FSofaNodeDefinition RootNode;
};

struct FSofaSceneLoadOptions
{
    bool bResolveRelativePaths = true;
    bool bNormalizeAttributes = true;

    FString ExternalScenesDirectory;
    FString RelativeScenesDirectory = TEXT("SofaScenes");
};

struct FSofaSceneLoadResult
{
    bool bSuccess = false;
    FString ErrorMessage;
    FSofaSceneDefinition SceneDefinition;
};

UCLASS()
class SOFABRIDGE_API USofaSceneLoader : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SOFA|Scene")
    static bool ProcessSceneFile(
        const FString& SceneFilePath,
        FString& OutResolvedPath,
        FString& OutErrorMessage);

    UFUNCTION(BlueprintCallable, Category = "SOFA|Scene")
    static bool ProcessSceneByName(
        const FString& SceneFileName,
        const FString& ExternalScenesDirectory,
        const FString& RelativeScenesDirectory,
        FString& OutResolvedPath,
        FString& OutErrorMessage);

    UFUNCTION(BlueprintPure, Category = "SOFA|Paths")
    static FString GetDefaultExternalScenesDirectory();

    UFUNCTION(BlueprintPure, Category = "SOFA|Paths")
    static FString GetDefaultRelativeScenesDirectory();

    UFUNCTION(BlueprintPure, Category = "SOFA|Paths")
    static FString GetExecutableRelativeScenesDirectoryAbsolute(
        const FString& RelativeScenesDirectory = TEXT("SofaScenes"));

    UFUNCTION(BlueprintPure, Category = "SOFA|Paths")
    static FString BuildSceneFilePath(
        const FString& DirectoryPath,
        const FString& SceneFileName);

    UFUNCTION(BlueprintPure, Category = "SOFA|Paths")
    static bool FileExists(const FString& FilePath);

    static bool LoadOverridesFromJsonFile(
        const FString& JsonFilePath,
        FSofaSceneIntegrationOverrides& OutOverrides,
        FString& OutError);

    static FSofaSceneLoadResult LoadSceneFromScnFile(
        const FString& SceneFilePath,
        const FSofaSceneLoadOptions& Options = FSofaSceneLoadOptions());

    static FSofaSceneLoadResult LoadSceneByName(
        const FString& SceneFileName,
        const FSofaSceneLoadOptions& Options = FSofaSceneLoadOptions());

    static const FSofaSceneDefinition* GetLastLoadedSceneDefinition();

private:
    static bool ResolveSceneFilePath(
        const FString& SceneFilePathOrName,
        const FSofaSceneLoadOptions& Options,
        FString& OutResolvedPath,
        FString& OutErrorMessage);

    static bool ResolveNamedSceneFilePath(
        const FString& SceneFileName,
        const FSofaSceneLoadOptions& Options,
        FString& OutResolvedPath,
        FString& OutErrorMessage);

    static bool ParseSceneDocument(
        const FString& SceneFilePath,
        const FSofaSceneLoadOptions& Options,
        FSofaSceneDefinition& OutSceneDefinition,
        FString& OutErrorMessage);

    static bool ParsePluginNode(
        const class FXmlNode* XmlNode,
        FSofaSceneDefinition& OutSceneDefinition,
        FString& OutErrorMessage);

    static bool ParseNodeElement(
        const class FXmlNode* XmlNode,
        const FString& SceneDirectory,
        const FSofaSceneLoadOptions& Options,
        FSofaNodeDefinition& OutNodeDefinition,
        FString& OutErrorMessage);

    static bool ParseComponentElement(
        const class FXmlNode* XmlNode,
        const FString& SceneDirectory,
        const FSofaSceneLoadOptions& Options,
        FSofaComponentDefinition& OutComponentDefinition,
        FString& OutErrorMessage);

    static void ExtractAttributes(
        const class FXmlNode* XmlNode,
        TMap<FString, FString>& OutAttributes);

    static bool IsPluginsNode(const class FXmlNode* XmlNode);
    static bool IsRequiredPluginElement(const class FXmlNode* XmlNode);
    static bool IsNodeElement(const class FXmlNode* XmlNode);
    static bool IsComponentElement(const class FXmlNode* XmlNode);

    static FString ResolveScenePath(
        const FString& InPath,
        const FString& SceneDirectory);

    static FString NormalizeDirectoryPath(const FString& InDirectoryPath);

    static void NormalizeAttributes(
        TMap<FString, FString>& InOutAttributes,
        const FString& SceneDirectory,
        const FSofaSceneLoadOptions& Options);

private:
    static TUniquePtr<FSofaSceneDefinition> LastLoadedSceneDefinition;
};