#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_HasGameplayTag.generated.h"

/**
 * Decorator to check if the AI pawn has a specific Gameplay Tag.
 */
UCLASS()
class CATTLEGAME_API UBTDecorator_HasGameplayTag : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HasGameplayTag();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

public:
	UPROPERTY(EditAnywhere, Category = "Condition")
	FGameplayTag GameplayTag;
};
