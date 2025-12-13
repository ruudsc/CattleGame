// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckNearbyThreats.generated.h"

/**
 * UBTService_CheckNearbyThreats
 *
 * Service that scans for nearby threats (players, predators) and updates blackboard.
 * Can add fear to the animal based on threat proximity.
 */
UCLASS()
class CATTLEGAME_API UBTService_CheckNearbyThreats : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_CheckNearbyThreats();

    virtual void TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key to store the nearest threat actor */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector NearestThreatKey;

    /** Key to store threat distance */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ThreatDistanceKey;

    /** Key to check if being lured (skip fear when true) */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsBeingLuredKey;

    /** Radius to scan for threats */
    UPROPERTY(EditAnywhere, Category = "Threat Detection", meta = (ClampMin = "100.0"))
    float ThreatDetectionRadius = 1500.0f;

    /** Fear added per second when threat is at minimum distance */
    UPROPERTY(EditAnywhere, Category = "Threat Detection", meta = (ClampMin = "0.0"))
    float MaxFearPerSecond = 20.0f;

    /** Distance at which fear addition starts */
    UPROPERTY(EditAnywhere, Category = "Threat Detection", meta = (ClampMin = "100.0"))
    float FearStartDistance = 1000.0f;

    /** Actor classes considered threats */
    UPROPERTY(EditAnywhere, Category = "Threat Detection")
    TArray<TSubclassOf<AActor>> ThreatClasses;

    /** Whether to consider players as threats */
    UPROPERTY(EditAnywhere, Category = "Threat Detection")
    bool bPlayersAreThreat = true;

    /** Find the nearest threat actor */
    AActor *FindNearestThreat(AAIController *AIController) const;
};
