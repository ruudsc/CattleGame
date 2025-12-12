// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CattleWander.generated.h"

/**
 * UBTTask_CattleWander
 *
 * Task that makes the cattle wander to a random location within range.
 * Uses the HomeLocation and WanderRadius from blackboard.
 */
UCLASS()
class CATTLEGAME_API UBTTask_CattleWander : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CattleWander();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for home location */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector HomeLocationKey;

    /** Key for wander radius */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector WanderRadiusKey;

    /** Key to store the target location */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    /** Minimum distance from current location for new wander point */
    UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "100.0"))
    float MinWanderDistance = 200.0f;

    /** Whether to use navigation system for valid points */
    UPROPERTY(EditAnywhere, Category = "Wander")
    bool bUseNavigation = true;
};
