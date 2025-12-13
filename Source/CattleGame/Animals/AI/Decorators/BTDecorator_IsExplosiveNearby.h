// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_IsExplosiveNearby.generated.h"

/**
 * BTDecorator_IsExplosiveNearby
 *
 * Returns true if there's an explosive (dynamite) nearby.
 * Checks if the NearbyExplosive blackboard key is set.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_IsExplosiveNearby : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_IsExplosiveNearby();

    virtual FString GetStaticDescription() const override;

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;

    /** Blackboard key for nearby explosive actor */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector NearbyExplosiveKey;
};
