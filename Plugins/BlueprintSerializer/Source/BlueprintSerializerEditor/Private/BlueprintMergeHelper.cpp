// Copyright CattleGame. All Rights Reserved.

#include "BlueprintMergeHelper.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintEditor.h"
#include "Merge.h"
#include "Modules/ModuleManager.h"

bool FBlueprintMergeHelper::OpenMergeEditor(UBlueprint *OriginalBlueprint, UBlueprint *ModifiedBlueprint)
{
    if (!OriginalBlueprint || !ModifiedBlueprint)
    {
        return false;
    }

    if (!IsMergeModuleAvailable())
    {
        UE_LOG(LogTemp, Error, TEXT("Merge module is not available"));
        return false;
    }

    // For a two-way merge, we use the original as both base and local
    return OpenThreeWayMergeEditor(OriginalBlueprint, OriginalBlueprint, ModifiedBlueprint);
}

bool FBlueprintMergeHelper::OpenThreeWayMergeEditor(UBlueprint *BaseBlueprint, UBlueprint *LocalBlueprint, UBlueprint *RemoteBlueprint)
{
    if (!BaseBlueprint || !LocalBlueprint || !RemoteBlueprint)
    {
        return false;
    }

    if (!IsMergeModuleAvailable())
    {
        UE_LOG(LogTemp, Error, TEXT("Merge module is not available"));
        return false;
    }

    // Get the Merge module
    IMerge &MergeModule = FModuleManager::LoadModuleChecked<IMerge>("Merge");

    // Open a Blueprint editor for the local Blueprint
    FBlueprintEditorModule &BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
    TSharedRef<IBlueprintEditor> BlueprintEditor = StaticCastSharedRef<IBlueprintEditor>(BlueprintEditorModule.CreateBlueprintEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), LocalBlueprint));

    // Cast to FBlueprintEditor to get the shared ref needed by the merge module
    TSharedRef<FBlueprintEditor> Editor = StaticCastSharedRef<FBlueprintEditor>(BlueprintEditor);

    // Define callback for when merge is resolved
    FOnMergeResolved OnMergeResolved = FOnMergeResolved::CreateLambda([](UPackage *MergedPackage, EMergeResult::Type Result)
                                                                      {
		if (Result == EMergeResult::Completed)
		{
			UE_LOG(LogTemp, Log, TEXT("Blueprint merge completed successfully"));
		}
		else if (Result == EMergeResult::Cancelled)
		{
			UE_LOG(LogTemp, Log, TEXT("Blueprint merge was cancelled"));
		} });

    // Open the three-way merge widget
    TSharedPtr<SDockTab> MergeTab = MergeModule.GenerateMergeWidget(BaseBlueprint, RemoteBlueprint, LocalBlueprint, OnMergeResolved, Editor);

    return MergeTab.IsValid();
}

FString FBlueprintMergeHelper::GetDiffSummary(const UBlueprint *BlueprintA, const UBlueprint *BlueprintB)
{
    if (!BlueprintA || !BlueprintB)
    {
        return TEXT("Invalid Blueprint(s) for comparison");
    }

    FString Summary;
    Summary += FString::Printf(TEXT("Comparing: %s vs %s\n"), *BlueprintA->GetName(), *BlueprintB->GetName());
    Summary += TEXT("---\n");

    // Compare variables
    {
        TSet<FName> VarsA, VarsB;
        for (const auto &Var : BlueprintA->NewVariables)
            VarsA.Add(Var.VarName);
        for (const auto &Var : BlueprintB->NewVariables)
            VarsB.Add(Var.VarName);

        TSet<FName> Added = VarsB.Difference(VarsA);
        TSet<FName> Removed = VarsA.Difference(VarsB);

        if (Added.Num() > 0)
        {
            Summary += TEXT("Variables Added:\n");
            for (const FName &Var : Added)
            {
                Summary += FString::Printf(TEXT("  + %s\n"), *Var.ToString());
            }
        }
        if (Removed.Num() > 0)
        {
            Summary += TEXT("Variables Removed:\n");
            for (const FName &Var : Removed)
            {
                Summary += FString::Printf(TEXT("  - %s\n"), *Var.ToString());
            }
        }
    }

    // Compare functions
    {
        TSet<FName> FuncsA, FuncsB;
        for (const TObjectPtr<UEdGraph> &Graph : BlueprintA->FunctionGraphs)
            if (Graph)
                FuncsA.Add(Graph->GetFName());
        for (const TObjectPtr<UEdGraph> &Graph : BlueprintB->FunctionGraphs)
            if (Graph)
                FuncsB.Add(Graph->GetFName());

        TSet<FName> Added = FuncsB.Difference(FuncsA);
        TSet<FName> Removed = FuncsA.Difference(FuncsB);

        if (Added.Num() > 0)
        {
            Summary += TEXT("Functions Added:\n");
            for (const FName &Func : Added)
            {
                Summary += FString::Printf(TEXT("  + %s\n"), *Func.ToString());
            }
        }
        if (Removed.Num() > 0)
        {
            Summary += TEXT("Functions Removed:\n");
            for (const FName &Func : Removed)
            {
                Summary += FString::Printf(TEXT("  - %s\n"), *Func.ToString());
            }
        }
    }

    // Node count comparison
    {
        int32 NodesA = 0, NodesB = 0;
        for (const TObjectPtr<UEdGraph> &Graph : BlueprintA->UbergraphPages)
            if (Graph)
                NodesA += Graph->Nodes.Num();
        for (const TObjectPtr<UEdGraph> &Graph : BlueprintB->UbergraphPages)
            if (Graph)
                NodesB += Graph->Nodes.Num();

        Summary += FString::Printf(TEXT("Event Graph Nodes: %d -> %d (%+d)\n"), NodesA, NodesB, NodesB - NodesA);
    }

    return Summary;
}

bool FBlueprintMergeHelper::IsMergeModuleAvailable()
{
    return FModuleManager::Get().IsModuleLoaded("Merge") || FModuleManager::Get().LoadModule("Merge") != nullptr;
}
