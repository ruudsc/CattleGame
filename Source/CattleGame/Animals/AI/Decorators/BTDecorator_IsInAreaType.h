// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "CattleGame/Animals/Areas/CattleAreaSubsystem.h"
#include "BTDecorator_IsInAreaType.generated.h"

/**
 * UBTDecorator_IsInAreaType
 *
 * Decorator that checks if the cattle is currently in a specific area type.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_IsInAreaType : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_IsInAreaType();

    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const override;
    virtual FString GetStaticDescription() const override;

protected:
    /** Key for current area type */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector CurrentAreaTypeKey;

    /** Area type to check for */
    UPROPERTY(EditAnywhere, Category = "Decorator")
    ECattleAreaType RequiredAreaType = ECattleAreaType::Graze;

    /** If true, check directly from subsystem instead of blackboard */
    UPROPERTY(EditAnywhere, Category = "Decorator")
    bool bCheckDirectly = false;
};
