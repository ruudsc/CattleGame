#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindGrazingPoint.generated.h"

/**
 * Task to find a reachable point within a Cattle Volume (Graze Logic).
 */
UCLASS()
class CATTLEGAME_API UBTTask_FindGrazingPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindGrazingPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	/** Blackboard Key to store the found location */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetLocationKey;

	/** Radius to search for random point if no volume found (fallback) */
	UPROPERTY(EditAnywhere, Category = "AI")
	float FallbackRadius = 500.0f;
};
