#pragma once
#include "Modules/ModuleManager.h"

class FSofaBridgeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};