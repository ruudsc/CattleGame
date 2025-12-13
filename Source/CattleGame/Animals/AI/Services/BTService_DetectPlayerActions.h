// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_DetectPlayerActions.generated.h"

/**
 * BTService_DetectPlayerActions
 *
 * Detects player actions affecting cattle and updates blackboard keys:
 * - Nearby explosives (dynamite)
 * - Trumpet lure/scare effects
 * - Player shooting nearby
 * - Direct gunshot hits
 */
UCLASS()
class CATTLEGAME_API UBTService_DetectPlayerActions : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_DetectPlayerActions();

    virtual void TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds) override;
    virtual FString GetStaticDescription() const override;

protected:
    // ===== BLACKBOARD KEYS =====

    /** Actor key for nearby explosive (dynamite projectile) */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector NearbyExplosiveKey;

    /** Bool key - is cattle being lured by trumpet */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsBeingLuredKey;

    /** Actor key - the player luring this cattle */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector LurerActorKey;

    /** Bool key - is cattle being scared by trumpet */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsBeingScaredKey;

    /** Actor key - the player scaring this cattle */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ScarerActorKey;

    /** Bool key - is player shooting nearby */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsPlayerShootingKey;

    /** Actor key - the player shooting */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ShooterActorKey;

    // ===== DETECTION SETTINGS =====

    /** Radius to detect nearby explosives */
    UPROPERTY(EditAnywhere, Category = "Detection|Explosive")
    float ExplosiveDetectionRadius = 800.0f;

    /** Radius to detect trumpet effects */
    UPROPERTY(EditAnywhere, Category = "Detection|Trumpet")
    float TrumpetDetectionRadius = 1500.0f;

    /** Radius to detect gunshots */
    UPROPERTY(EditAnywhere, Category = "Detection|Shooting")
    float GunshotDetectionRadius = 1500.0f;

    /** Time after last gunshot to still consider "shooting" */
    UPROPERTY(EditAnywhere, Category = "Detection|Shooting")
    float GunshotMemoryTime = 2.0f;

private:
    void DetectNearbyExplosives(UBehaviorTreeComponent &OwnerComp, class ACattleAnimal *Animal, class UBlackboardComponent *BlackboardComp);
    void DetectTrumpetEffects(UBehaviorTreeComponent &OwnerComp, class ACattleAnimal *Animal, class UBlackboardComponent *BlackboardComp);
    void DetectPlayerShooting(UBehaviorTreeComponent &OwnerComp, class ACattleAnimal *Animal, class UBlackboardComponent *BlackboardComp);
};
