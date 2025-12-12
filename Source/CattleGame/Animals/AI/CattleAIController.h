// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CattleAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class ACattleAnimal;

/**
 * ACattleAIController
 *
 * AI Controller for cattle animals using Behavior Trees.
 * Manages blackboard data and behavior tree execution.
 */
UCLASS(Blueprintable, BlueprintType)
class CATTLEGAME_API ACattleAIController : public AAIController
{
    GENERATED_BODY()

public:
    ACattleAIController();

    // ===== Blackboard Keys =====

    static const FName KEY_TargetLocation;
    static const FName KEY_TargetActor;
    static const FName KEY_CurrentAreaType;
    static const FName KEY_FearLevel;
    static const FName KEY_IsPanicked;
    static const FName KEY_FlowDirection;
    static const FName KEY_HomeLocation;
    static const FName KEY_WanderRadius;

    // ===== Getters =====

    /** Get the controlled cattle animal */
    UFUNCTION(BlueprintCallable, Category = "Cattle AI")
    ACattleAnimal *GetCattleAnimal() const;

    /** Get the behavior tree component */
    UFUNCTION(BlueprintCallable, Category = "Cattle AI")
    UBehaviorTreeComponent *GetBehaviorTreeComponent() const { return BehaviorTreeComp; }

    /** Get the blackboard component */
    UBlackboardComponent *GetBlackboardComp() const { return BlackboardComp; }

    // ===== Blackboard Helpers =====

    /** Update fear-related blackboard values */
    UFUNCTION(BlueprintCallable, Category = "Cattle AI")
    void UpdateFearBlackboard();

    /** Update area-related blackboard values */
    UFUNCTION(BlueprintCallable, Category = "Cattle AI")
    void UpdateAreaBlackboard();

    /** Set target location for movement */
    UFUNCTION(BlueprintCallable, Category = "Cattle AI")
    void SetTargetLocation(const FVector &Location);

    /** Set target actor */
    UFUNCTION(BlueprintCallable, Category = "Cattle AI")
    void SetTargetActor(AActor *Actor);

    /** Set the home location (spawn point) */
    UFUNCTION(BlueprintCallable, Category = "Cattle AI")
    void SetHomeLocation(const FVector &Location);

protected:
    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnUnPossess() override;
    virtual void BeginPlay() override;

    // ===== Components =====

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBlackboardComponent> BlackboardComp;

    // ===== Configuration =====

    /** Behavior tree to run */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cattle AI")
    TObjectPtr<UBehaviorTree> BehaviorTree;

    /** Default wander radius around home location */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cattle AI")
    float DefaultWanderRadius = 1000.0f;

private:
    /** Initialize blackboard with default values for cattle AI */
    void InitializeCattleBlackboard();
};
