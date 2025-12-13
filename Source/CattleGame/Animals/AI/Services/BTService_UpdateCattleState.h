// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCattleState.generated.h"

/**
 * UBTService_UpdateCattleState
 *
 * Service that continuously updates the blackboard with cattle state.
 * Updates fear level, panic state, and area influences.
 */
UCLASS()
class CATTLEGAME_API UBTService_UpdateCattleState : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_UpdateCattleState();

    virtual void TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for fear level */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FearLevelKey;

    /** Key for panic state */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsPanickedKey;

    /** Key for current area type */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector CurrentAreaTypeKey;

    /** Key for flow direction */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector FlowDirectionKey;

    /** Whether to update the animal's area influences */
    UPROPERTY(EditAnywhere, Category = "Service")
    bool bUpdateAreaInfluences = true;
};
