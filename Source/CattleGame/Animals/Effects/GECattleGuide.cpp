#include "GECattleGuide.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGECattleGuide::UGECattleGuide()
{
	FGameplayTag GuideTag = CattleGameplayTags::State_Cattle_Guided;

	// Use 5.3+ Component Setup
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(GuideTag);
	TargetTagsComponent->SetAndApplyTargetTagChanges(TagContainer);
}
