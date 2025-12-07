#include "BTService_CattleState.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTService_CattleState::UBTService_CattleState()
{
	NodeName = "Update Cattle State from Tag";
	bNotifyTick = true;
	Interval = 0.2f; // Check every 0.2s
	RandomDeviation = 0.05f;
}

void UBTService_CattleState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (!bUpdateBlackboardDetails)
	{
		return;
	}

	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller) return;

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (ASC)
	{
		bool bHasTag = ASC->HasMatchingGameplayTag(GameplayTag);
		
		UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
		if (BB)
		{
			BB->SetValueAsBool(GetSelectedBlackboardKey(), bHasTag);
		}
	}
}
