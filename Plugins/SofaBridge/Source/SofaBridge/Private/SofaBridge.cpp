#include "SofaBridge.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

IMPLEMENT_MODULE(FSofaBridgeModule, SofaBridge)

void FSofaBridgeModule::StartupModule()
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SofaBridge"));
    if (!Plugin.IsValid())
    {
        return;
    }

    TArray<FString> CoreDlls = {
        TEXT("Sofa.Helper"),
        TEXT("Sofa.Type"),
        TEXT("Sofa.DefaultType"),
        TEXT("Sofa.Core"),
        TEXT("Sofa.Simulation.Core"),
        TEXT("Sofa.Simulation.Common"),
        TEXT("Sofa.Simulation.Graph"),
        TEXT("Sofa.SimpleApi"),
        TEXT("Sofa.LinearAlgebra"),
        TEXT("Sofa.Geometry"),
    };

    TArray<FString> SofaRuntimePlugins = {
        TEXT("Sofa.Component.ODESolver.Backward"),
        TEXT("Sofa.Component.LinearSolver.Direct"),
        TEXT("Sofa.Component.LinearSolver.Iterative"),
        TEXT("Sofa.Component.LinearSolver.Ordering"),
        TEXT("Sofa.Component.LinearSystem"),
        TEXT("Sofa.Component.StateContainer"),
        TEXT("Sofa.Component.Mass"),
        TEXT("Sofa.Component.IO.Mesh"),
        TEXT("Sofa.Component.Topology.Container.Grid"),
        TEXT("Sofa.Component.Topology.Container.Dynamic"),
        TEXT("Sofa.Component.Topology.Container.Constant"),
        TEXT("Sofa.Component.SolidMechanics.FEM.Elastic"),
    };

    for (const FString& DllName : CoreDlls)
    {
        FString FullPath = FPaths::Combine(SofaDllDirectory, DllName) + ".dll";
        void* Handle = FPlatformProcess::GetDllHandle(*FullPath);

        if (!Handle)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load SOFA DLL: %s"), *FullPath);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Loaded SOFA DLL: %s"), *DllName);
        }
    }
}

void FSofaBridgeModule::ShutdownModule()
{
    if (!SofaDllDirectory.IsEmpty())
    {
        FPlatformProcess::PopDllDirectory(*SofaDllDirectory);
        SofaDllDirectory.Reset();
    }
}