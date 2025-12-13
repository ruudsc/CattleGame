// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CattleAreaBase.h"
#include "CattleAvoidArea.generated.h"

/**
 * ACattleAvoidArea
 *
 * Area that cattle will avoid entering. Animals will:
 * - Be pushed away from the area boundary
 * - Pathfind around the area when possible
 * - Not panic, just calmly avoid
 *
 * Use for obstacles, cliffs, fences, water, etc.
 * Place in level and search "CattleAvoid" in actor panel.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Cattle Avoid Area"))
class CATTLEGAME_API ACattleAvoidArea : public ACattleAreaBase
{
    GENERATED_BODY()

public:
    ACattleAvoidArea();

    // ===== Area Type =====

    virtual ECattleAreaType GetAreaType() const override { return ECattleAreaType::Avoid; }

    // ===== Avoid Configuration =====

    /** How strongly animals are pushed away from this area */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Avoid", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float AvoidStrength = 1.0f;

    /** Distance from boundary where avoidance starts (outside the area) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Avoid", meta = (ClampMin = "0.0"))
    float AvoidanceRadius = 300.0f;

    /** Whether to completely block movement into this area */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Avoid")
    bool bHardBoundary = false;

    /** Speed reduction when inside (if forced in) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Avoid", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float InsideSpeedModifier = 0.5f;

    // ===== Overrides =====

    virtual bool IsLocationInArea(const FVector &Location) const override;
    virtual FCattleAreaInfluence GetInfluenceAtLocation(const FVector &Location) const override;
    virtual float GetSpeedModifier() const override { return InsideSpeedModifier; }
    virtual FVector GetInfluenceDirection(const FVector &Location) const override;

protected:
    /** Get direction away from area boundary */
    FVector GetAvoidanceDirection(const FVector &Location) const;
};
