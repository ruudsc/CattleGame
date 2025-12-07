#include "BTDecorator_HasGameplayTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"

UBTDecorator_HasGameplayTag::UBTDecorator_HasGameplayTag()
{
	NodeName = "Has Gameplay Tag";
}

bool UBTDecorator_HasGameplayTag::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller) return false;

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn) return false;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (ASC)
	{
		return ASC->HasMatchingGameplayTag(GameplayTag);
	}

	return false;
}
