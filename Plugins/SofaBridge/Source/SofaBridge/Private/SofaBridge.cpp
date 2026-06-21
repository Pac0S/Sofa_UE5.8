#include "SofaBridge.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

IMPLEMENT_MODULE(FSofaBridgeModule, SofaBridge)

void FSofaBridgeModule::StartupModule()
{
    FString PluginBaseDir = IPluginManager::Get().FindPlugin("SofaBridge")->GetBaseDir();
    FString DllDir = FPaths::Combine(PluginBaseDir, TEXT("Binaries"), TEXT("Win64"));

    TArray<FString> SofaDlls = {
        TEXT("Sofa.Helper.dll"),
        TEXT("Sofa.Type.dll"),
        TEXT("Sofa.DefaultType.dll"),
        TEXT("Sofa.Core.dll"),
        TEXT("Sofa.Simulation.Core.dll"),
        TEXT("Sofa.Simulation.Common.dll"),
        TEXT("Sofa.Simulation.Graph.dll"),
        TEXT("Sofa.SimpleApi.dll"),
    };

    for (const FString& DllName : SofaDlls)
    {
        FString FullPath = FPaths::Combine(DllDir, DllName);
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
}