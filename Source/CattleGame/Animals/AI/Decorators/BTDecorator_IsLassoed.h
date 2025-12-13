// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsLassoed.generated.h"

/**
 * BTDecorator_IsLassoed
 *
 * Returns true if the cattle animal is currently captured by a lasso.
 * Use with "Inverse Condition" to block behaviors while lassoed.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_IsLassoed : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_IsLassoed();

    virtual FString GetStaticDescription() const override;

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;
};
