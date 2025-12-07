#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CattleAIController.generated.h"

class UBehaviorTree;

/**
 * AI Controller for Cattle.
 * Runs the Behavior Tree and initializes the GAS if needed.
 */
UCLASS()
class CATTLEGAME_API ACattleAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACattleAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	/** The Behavior Tree asset to run */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
