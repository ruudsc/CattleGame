// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_LookAtActor.generated.h"

/**
 * BTTask_LookAtActor
 *
 * Makes the AI look at a blackboard actor without moving.
 * Used for alert/curious state when player is shooting nearby.
 */
UCLASS()
class CATTLEGAME_API UBTTask_LookAtActor : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_LookAtActor();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Blackboard key for the actor to look at */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ActorToLookAtKey;

    /** How fast to turn (degrees per second). 0 = instant */
    UPROPERTY(EditAnywhere, Category = "Look At")
    float TurnSpeed = 180.0f;
};
