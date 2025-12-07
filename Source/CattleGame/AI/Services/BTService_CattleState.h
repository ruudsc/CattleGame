#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "GameplayTagContainer.h"
#include "BTService_CattleState.generated.h"

/**
 * Service to sync Gameplay Tags (State.Cattle.*) to Blackboard Keys.
 * Allows the Behavior Tree to react to GAS changes.
 */
UCLASS()
class CATTLEGAME_API UBTService_CattleState : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_CattleState();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	/** Tag to check for */
	UPROPERTY(EditAnywhere, Category = "Cattle Logic")
	FGameplayTag GameplayTag;

	/** If true, the Blackboard Key (boolean) will be set to match the Tag presence. */
	UPROPERTY(EditAnywhere, Category = "Cattle Logic")
	bool bUpdateBlackboardDetails = true;
};
