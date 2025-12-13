// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FleeFromActor.generated.h"

/**
 * BTTask_FleeFromActor
 *
 * Sets TargetLocation to flee away from a blackboard actor.
 * Used for fleeing from dynamite, trumpet scare, shooter, etc.
 */
UCLASS()
class CATTLEGAME_API UBTTask_FleeFromActor : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FleeFromActor();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Blackboard key for the actor to flee from */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ActorToFleeFromKey;

    /** Blackboard key for output target location */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    /** How far to flee */
    UPROPERTY(EditAnywhere, Category = "Flee")
    float FleeDistance = 800.0f;

    /** Random angle variation (degrees) */
    UPROPERTY(EditAnywhere, Category = "Flee")
    float RandomAngleVariation = 30.0f;

    /** Whether to use navigation for valid locations */
    UPROPERTY(EditAnywhere, Category = "Flee")
    bool bUseNavigation = true;
};
