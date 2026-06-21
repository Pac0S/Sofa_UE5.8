using UnrealBuildTool;
using System.IO;

public class Boost : ModuleRules
{
    public Boost(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string BoostRoot = ModuleDirectory;
        PublicSystemIncludePaths.Add(Path.Combine(BoostRoot, "include"));
    }
}