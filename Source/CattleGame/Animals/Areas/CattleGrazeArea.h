// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CattleAreaBase.h"
#include "CattleGrazeArea.generated.h"

/**
 * ACattleGrazeArea
 *
 * Area where cattle prefer to graze. Animals inside will:
 * - Move slowly and periodically stop to graze
 * - Have reduced fear buildup
 * - Stay within the area when possible
 *
 * Place in level and search "CattleGraze" in actor panel.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Cattle Graze Area"))
class CATTLEGAME_API ACattleGrazeArea : public ACattleAreaBase
{
    GENERATED_BODY()

public:
    ACattleGrazeArea();

    // ===== Area Type =====

    virtual ECattleAreaType GetAreaType() const override { return ECattleAreaType::Graze; }

    // ===== Graze Configuration =====

    /** Movement speed modifier when grazing (0.1 = 10% speed) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Graze", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float GrazeSpeedModifier = 0.3f;

    /** Fear decay multiplier in this area (higher = fear reduces faster) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Graze", meta = (ClampMin = "1.0", ClampMax = "5.0"))
    float FearDecayMultiplier = 2.0f;

    /** How strongly animals are encouraged to stay in the graze area */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Graze", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ContainmentStrength = 0.5f;

    // ===== Overrides =====

    virtual float GetSpeedModifier() const override { return GrazeSpeedModifier; }
    virtual FVector GetInfluenceDirection(const FVector &Location) const override;

protected:
    /** Get direction toward center of area to encourage staying */
    FVector GetContainmentDirection(const FVector &Location) const;
};
