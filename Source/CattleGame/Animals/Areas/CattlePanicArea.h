// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CattleAreaBase.h"
#include "CattlePanicArea.generated.h"

/**
 * ACattlePanicArea
 *
 * Area that causes cattle to panic and flee. Animals inside will:
 * - Gain fear rapidly
 * - Run at maximum speed
 * - Flee away from the area center
 *
 * Use for predator zones, loud noises, fire, etc.
 * Place in level and search "CattlePanic" in actor panel.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Cattle Panic Area"))
class CATTLEGAME_API ACattlePanicArea : public ACattleAreaBase
{
    GENERATED_BODY()

public:
    ACattlePanicArea();

    // ===== Area Type =====

    virtual ECattleAreaType GetAreaType() const override { return ECattleAreaType::Panic; }

    // ===== Panic Configuration =====

    /** Movement speed modifier when panicking (1.5 = 150% speed) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Panic", meta = (ClampMin = "1.0", ClampMax = "3.0"))
    float PanicSpeedModifier = 1.5f;

    /** Amount of fear added per second while in area */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Panic", meta = (ClampMin = "0.0"))
    float FearPerSecond = 30.0f;

    /** How strongly animals flee from the area center */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Panic", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FleeStrength = 1.0f;

    /** Whether the flee direction should be random (stampede behavior) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Panic")
    bool bRandomFleeDirection = false;

    // ===== Overrides =====

    virtual float GetSpeedModifier() const override { return PanicSpeedModifier; }
    virtual FVector GetInfluenceDirection(const FVector &Location) const override;

protected:
    /** Get direction away from area center/threat */
    FVector GetFleeDirection(const FVector &Location) const;
};
