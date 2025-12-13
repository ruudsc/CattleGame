// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_FearLevel.generated.h"

/**
 * UBTDecorator_FearLevel
 *
 * Decorator that checks if fear level is within a specific range.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_FearLevel : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_FearLevel();

    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for fear level (0-1) */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FearLevelKey;

    /** Minimum fear level to pass (0-1) */
    UPROPERTY(EditAnywhere, Category = "Decorator", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinFearLevel = 0.0f;

    /** Maximum fear level to pass (0-1) */
    UPROPERTY(EditAnywhere, Category = "Decorator", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxFearLevel = 1.0f;
};
