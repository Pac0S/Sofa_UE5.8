using UnrealBuildTool;
using System;
using System.IO;
using System.Linq;

public class SofaSDK : ModuleRules
{
    public SofaSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        bUseUnity = false;

        if (Target.Platform != UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("SOFA_SDK_ENABLED=0");
            return;
        }

        PublicDefinitions.Add("_USE_MATH_DEFINES");
        PublicDefinitions.Add("NOMINMAX");
        PublicDefinitions.Add("SOFA_SDK_ENABLED=1");

        //Include SOFA headers and link to SOFA libraries
        string SofaRoot = Environment.GetEnvironmentVariable("SOFA_ROOT");
        if (string.IsNullOrEmpty(SofaRoot))
        {
            SofaRoot = Path.Combine(ModuleDirectory, "SOFA");
        }
        if (!Directory.Exists(SofaRoot))
        {
            throw new BuildException($"SOFA directory not found: {SofaRoot}");
        }
        string IncludeDir = Path.Combine(SofaRoot, "include");
        string LibDir = Path.Combine(SofaRoot, "lib");
        string BinDir = Path.Combine(SofaRoot, "bin");

        if (!Directory.Exists(IncludeDir))
        {
            throw new BuildException("SOFA include directory not found: " + IncludeDir);
        }

        if (!Directory.Exists(LibDir))
        {
            throw new BuildException("SOFA lib directory not found: " + LibDir);
        }

        if (!Directory.Exists(BinDir))
        {
            throw new BuildException("SOFA bin directory not found: " + BinDir);
        }

        PublicIncludePaths.Add(IncludeDir);

        foreach (string moduleIncludeDir in Directory.GetDirectories(IncludeDir, "Sofa.*"))
        {
            PublicIncludePaths.Add(moduleIncludeDir);
        }

        string[] SofaModules =
        {
            "Sofa.Core",
            "Sofa.Helper",
            "Sofa.Type",
            "Sofa.DefaultType",
            "Sofa.Simulation.Core",
            "Sofa.Simulation.Common",
            "Sofa.Simulation.Graph",
            "Sofa.SimpleApi",
            "Sofa.Component.ODESolver.Backward",
            "Sofa.Component.LinearSolver.Direct",
            "Sofa.Component.LinearSolver.Iterative",
            "Sofa.Component.LinearSolver.Ordering",
            "Sofa.Component.LinearSystem",
            "Sofa.Component.StateContainer",
            "Sofa.Component.Mass",
            "Sofa.Component.SolidMechanics.FEM.Elastic",
            "Sofa.Component.IO.Mesh",
            "Sofa.Component.Topology.Container.Dynamic",
            "Sofa.Component.Topology.Container.Grid",
            "Sofa.Component.Topology.Container.Constant",
            "Sofa.Component.Topology.Mapping",
            "Sofa.Component.Constraint.Projective",
            "Sofa.Component.MechanicalLoad",
            "Sofa.Component.Mapping.Linear",
            "Sofa.LinearAlgebra",
            "Sofa.Geometry",
            "Sofa.GL.Component.Rendering3D",
            "tinyxml2"
        };

        foreach (string moduleName in SofaModules.Distinct())
        {
            string libPath = Path.Combine(LibDir, moduleName + ".lib");
            if (!File.Exists(libPath))
            {
                throw new BuildException("Missing SOFA import library: " + libPath);
            }

            PublicAdditionalLibraries.Add(libPath);

            string dllName = moduleName + ".dll";
            string dllSourcePath = Path.Combine(BinDir, dllName);

            if (File.Exists(dllSourcePath))
            {
                RuntimeDependencies.Add(
                    Path.Combine("$(BinaryOutputDir)", dllName),
                    dllSourcePath
                );
            }
        }

        //Include Boost headers
        string BoostRoot = Environment.GetEnvironmentVariable("BOOST_ROOT");
        if (string.IsNullOrEmpty(BoostRoot))
        {
            BoostRoot = Path.Combine(ModuleDirectory, "Boost");
        }

        if (!Directory.Exists(BoostRoot))
        {
            throw new BuildException("Boost root directory not found: " + BoostRoot);
        }

        PublicSystemIncludePaths.Add(BoostRoot);
    }
}