// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Forward declaration for subsystem
class UBlueprintSerializerSubsystem;

class FBlueprintSerializerEditorModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    /** Register Content Browser context menu extensions */
    void RegisterMenuExtensions();

    /** Unregister menu extensions */
    void UnregisterMenuExtensions();

    /** Register console commands */
    void RegisterConsoleCommands();

    /** Unregister console commands */
    void UnregisterConsoleCommands();

    /** Handle for the registered extension delegate */
    FDelegateHandle ContentBrowserExtenderDelegateHandle;

    /** Registered console commands */
    TArray<IConsoleObject *> ConsoleCommands;
};
