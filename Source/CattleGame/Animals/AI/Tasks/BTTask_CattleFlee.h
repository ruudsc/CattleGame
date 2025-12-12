// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CattleFlee.generated.h"

/**
 * UBTTask_CattleFlee
 *
 * Task that makes the cattle flee from a threat or in a direction.
 * Uses area influence direction or flees from a target actor.
 */
UCLASS()
class CATTLEGAME_API UBTTask_CattleFlee : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CattleFlee();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for flow/flee direction from area */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FlowDirectionKey;

    /** Key for target actor to flee from (optional) */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ThreatActorKey;

    /** Key to store the flee target location */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    /** Distance to flee */
    UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "100.0"))
    float FleeDistance = 500.0f;

    /** Add random angle variation to flee direction */
    UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float RandomAngleVariation = 15.0f;
};
