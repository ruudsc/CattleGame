#include "GECattleAvoid.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGECattleAvoid::UGECattleAvoid()
{
	FGameplayTag AvoidTag = CattleGameplayTags::State_Cattle_Avoiding;

	// Use 5.3+ Component Setup
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(AvoidTag);
	TargetTagsComponent->SetAndApplyTargetTagChanges(TagContainer);
}
