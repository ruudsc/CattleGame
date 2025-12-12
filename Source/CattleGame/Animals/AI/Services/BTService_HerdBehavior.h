// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_HerdBehavior.generated.h"

/**
 * UBTService_HerdBehavior
 *
 * Service that influences cattle movement based on nearby herd members.
 * Implements cohesion, alignment, and separation behaviors.
 */
UCLASS()
class CATTLEGAME_API UBTService_HerdBehavior : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_HerdBehavior();

    virtual void TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key to store herd center direction */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector HerdDirectionKey;

    /** Key to store number of nearby herd members */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector HerdCountKey;

    /** Radius to detect herd members */
    UPROPERTY(EditAnywhere, Category = "Herd", meta = (ClampMin = "100.0"))
    float HerdRadius = 800.0f;

    /** Minimum separation distance from other herd members */
    UPROPERTY(EditAnywhere, Category = "Herd", meta = (ClampMin = "50.0"))
    float SeparationDistance = 150.0f;

    /** Weight for cohesion (moving toward herd center) */
    UPROPERTY(EditAnywhere, Category = "Herd|Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CohesionWeight = 0.3f;

    /** Weight for alignment (matching herd direction) */
    UPROPERTY(EditAnywhere, Category = "Herd|Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AlignmentWeight = 0.2f;

    /** Weight for separation (avoiding crowding) */
    UPROPERTY(EditAnywhere, Category = "Herd|Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SeparationWeight = 0.5f;

private:
    /** Find all nearby herd members */
    TArray<class ACattleAnimal *> FindNearbyHerdMembers(ACattleAnimal *Animal) const;
};
