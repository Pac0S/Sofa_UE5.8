using UnrealBuildTool;
using System.IO;

public class SofaSDK : ModuleRules
{
    public SofaSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string SofaDir      = Path.Combine(ModuleDirectory, "SOFA");
        string IncludeDir   = Path.Combine(SofaDir, "include");
        string LibDir       = Path.Combine(SofaDir, "lib", "Win64");
        string BinDir       = Path.Combine(SofaDir, "bin", "Win64");

        if (Directory.Exists(IncludeDir))
        {
            PublicIncludePaths.Add(IncludeDir);

            foreach (string moduleIncludeDir in Directory.GetDirectories(IncludeDir, "Sofa.*"))
            {
                PublicIncludePaths.Add(moduleIncludeDir);
            }
        }

        PublicDependencyModuleNames.Add("Boost");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("_USE_MATH_DEFINES");
            PublicDefinitions.Add("NOMINMAX");
            PublicDefinitions.Add("SOFA_SDK_ENABLED=1");

            bUseUnity = false;
            
            if (Directory.Exists(LibDir))
            {
                foreach (string libPath in Directory.GetFiles(LibDir, "*.lib"))
                {
                    PublicAdditionalLibraries.Add(libPath);
                }
            }

            if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                if (Directory.Exists(BinDir))
                {
                    foreach (string dllSourcePath in Directory.GetFiles(BinDir, "*.dll"))
                    {
                        string dllName = Path.GetFileName(dllSourcePath);
                        string Destination = Path.Combine("$(BinaryOutputDir)", dllName);
                        string Source = Path.Combine("$(PluginDir)", "Source/ThirdParty/SofaSDK/SOFA/bin/Win64", dllName);

                        RuntimeDependencies.Add(Destination, Source);
                        PublicDelayLoadDLLs.Add(dllName);
                    }
                }
            }
        }
        else
        {
            PublicDefinitions.Add("SOFA_SDK_ENABLED=0");
        }
    }
}