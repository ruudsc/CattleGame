// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasNearbyThreat.generated.h"

/**
 * UBTDecorator_HasNearbyThreat
 *
 * Decorator that checks if there's a threat within a certain distance.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_HasNearbyThreat : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_HasNearbyThreat();

    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for nearest threat actor */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector NearestThreatKey;

    /** Key for threat distance */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ThreatDistanceKey;

    /** Maximum distance to consider threat as "nearby" */
    UPROPERTY(EditAnywhere, Category = "Decorator", meta = (ClampMin = "100.0"))
    float MaxThreatDistance = 1000.0f;
};
