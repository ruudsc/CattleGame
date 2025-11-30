// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlueprintSerializerModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Get the module instance */
    static FBlueprintSerializerModule &Get()
    {
        return FModuleManager::LoadModuleChecked<FBlueprintSerializerModule>("BlueprintSerializer");
    }

    /** Check if module is loaded */
    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("BlueprintSerializer");
    }
};
