// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CattleFollowFlow.generated.h"

/**
 * UBTTask_CattleFollowFlow
 *
 * Task that makes the cattle move in the direction of area flow.
 * Used for guiding cattle along flow guides.
 */
UCLASS()
class CATTLEGAME_API UBTTask_CattleFollowFlow : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CattleFollowFlow();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for flow direction */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FlowDirectionKey;

    /** Key to store the target location */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    /** Distance to move in flow direction */
    UPROPERTY(EditAnywhere, Category = "Flow", meta = (ClampMin = "100.0"))
    float FlowDistance = 300.0f;
};
