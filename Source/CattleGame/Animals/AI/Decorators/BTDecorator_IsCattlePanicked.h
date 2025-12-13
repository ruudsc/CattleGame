// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsCattlePanicked.generated.h"

/**
 * UBTDecorator_IsCattlePanicked
 *
 * Decorator that checks if the cattle is currently panicked.
 * Returns true if fear level is above panic threshold.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_IsCattlePanicked : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_IsCattlePanicked();

    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for panic state */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsPanickedKey;

    /** If true, check directly from animal instead of blackboard */
    UPROPERTY(EditAnywhere, Category = "Decorator")
    bool bCheckDirectly = false;
};
