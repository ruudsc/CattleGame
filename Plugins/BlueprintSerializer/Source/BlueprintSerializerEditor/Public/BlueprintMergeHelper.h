// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;

/**
 * Utility class for Blueprint merge operations using UE5's IMerge module
 */
class BLUEPRINTSERIALIZEREDITOR_API FBlueprintMergeHelper
{
public:
    /**
     * Open the three-way merge editor for two Blueprint assets
     * @param OriginalBlueprint The original Blueprint (base)
     * @param ModifiedBlueprint The modified Blueprint (from AI/JSON import)
     * @return True if the merge editor was opened successfully
     */
    static bool OpenMergeEditor(UBlueprint *OriginalBlueprint, UBlueprint *ModifiedBlueprint);

    /**
     * Open the full three-way merge editor
     * @param BaseBlueprint The common ancestor Blueprint
     * @param LocalBlueprint The local version of the Blueprint
     * @param RemoteBlueprint The remote/modified version of the Blueprint
     * @return True if the merge editor was opened successfully
     */
    static bool OpenThreeWayMergeEditor(UBlueprint *BaseBlueprint, UBlueprint *LocalBlueprint, UBlueprint *RemoteBlueprint);

    /**
     * Get the diff between two Blueprints as a human-readable summary
     * @param BlueprintA First Blueprint
     * @param BlueprintB Second Blueprint
     * @return String describing the differences
     */
    static FString GetDiffSummary(const UBlueprint *BlueprintA, const UBlueprint *BlueprintB);

    /**
     * Check if the Merge module is available
     * @return True if merge functionality is available
     */
    static bool IsMergeModuleAvailable();
};
