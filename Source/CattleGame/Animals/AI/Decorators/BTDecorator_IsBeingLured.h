// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_IsBeingLured.generated.h"

/**
 * BTDecorator_IsBeingLured
 *
 * Returns true if the cattle is being lured by a trumpet.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_IsBeingLured : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_IsBeingLured();

    virtual FString GetStaticDescription() const override;

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;

    /** Blackboard key for IsBeingLured bool */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsBeingLuredKey;

    /** Optional: Only consider lured if fear is below this threshold (0.0-1.0). Set to 1.0 to ignore. */
    UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxFearForLure = 0.3f;

    /** Blackboard key for current fear level (optional) */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FearLevelKey;
};
