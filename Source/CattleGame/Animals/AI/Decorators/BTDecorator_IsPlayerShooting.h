// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_IsPlayerShooting.generated.h"

/**
 * BTDecorator_IsPlayerShooting
 *
 * Returns true if a player is shooting nearby.
 * Can optionally check fear level to differentiate reactions.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_IsPlayerShooting : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_IsPlayerShooting();

    virtual FString GetStaticDescription() const override;

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;

    /** Blackboard key for IsPlayerShooting bool */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsPlayerShootingKey;

    /** Blackboard key for current fear level */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FearLevelKey;

    /** If true, only match when fear is above MinFearLevel */
    UPROPERTY(EditAnywhere, Category = "Condition")
    bool bRequireMinFear = false;

    /** Minimum fear level required (0.0-1.0) */
    UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bRequireMinFear"))
    float MinFearLevel = 0.3f;

    /** If true, only match when fear is below MaxFearLevel */
    UPROPERTY(EditAnywhere, Category = "Condition")
    bool bRequireMaxFear = false;

    /** Maximum fear level allowed (0.0-1.0) */
    UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bRequireMaxFear"))
    float MaxFearLevel = 0.3f;
};
