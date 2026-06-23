using UnrealBuildTool;

public class SofaBridge : ModuleRules
{
    public SofaBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bUseRTTI = true;
        bUseUnity = false;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            bEnableExceptions = true;
            bWarningsAsErrors = false;
        }

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Projects",
            "SofaSDK"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate",
            "SlateCore"
        });
    }
}