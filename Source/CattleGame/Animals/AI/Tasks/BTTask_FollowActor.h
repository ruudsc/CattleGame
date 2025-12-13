// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FollowActor.generated.h"

/**
 * BTTask_FollowActor
 *
 * Sets TargetLocation to move toward a blackboard actor.
 * Used for trumpet lure - cattle follow the player.
 */
UCLASS()
class CATTLEGAME_API UBTTask_FollowActor : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FollowActor();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Blackboard key for the actor to follow */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ActorToFollowKey;

    /** Blackboard key for output target location */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    /** How close to get to the actor (stop distance) */
    UPROPERTY(EditAnywhere, Category = "Follow")
    float AcceptableRadius = 300.0f;

    /** Movement speed multiplier (1.0 = normal) */
    UPROPERTY(EditAnywhere, Category = "Follow")
    float SpeedMultiplier = 0.6f;
};
