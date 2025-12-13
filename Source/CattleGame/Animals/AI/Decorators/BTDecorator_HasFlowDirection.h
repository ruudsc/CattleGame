// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasFlowDirection.generated.h"

/**
 * UBTDecorator_HasFlowDirection
 *
 * Decorator that checks if there's a valid flow direction to follow.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_HasFlowDirection : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_HasFlowDirection();

    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for flow direction */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FlowDirectionKey;

    /** Minimum magnitude to consider as valid flow */
    UPROPERTY(EditAnywhere, Category = "Decorator", meta = (ClampMin = "0.0"))
    float MinimumMagnitude = 0.1f;
};
