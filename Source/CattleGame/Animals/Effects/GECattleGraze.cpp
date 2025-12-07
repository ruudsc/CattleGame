#include "GECattleGraze.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGECattleGraze::UGECattleGraze()
{
	FGameplayTag GrazeTag = CattleGameplayTags::State_Cattle_Grazing;
	
	// Use 5.3+ Component Setup
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComponent);
	
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(GrazeTag);
	TargetTagsComponent->SetAndApplyTargetTagChanges(TagContainer);
}
