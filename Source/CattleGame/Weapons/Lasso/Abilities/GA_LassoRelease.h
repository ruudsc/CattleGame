#pragma once

#include "CoreMinimal.h"
#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_LassoRelease.generated.h"

class ALasso;

/**
 * Lasso Release Ability - Secondary fire to release tether.
 *
 * Behavior:
 * - If tethered: Release target and start retracting
 * - Otherwise: No effect
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_LassoRelease : public UGA_Weapon
{
	GENERATED_BODY()

public:
	UGA_LassoRelease();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	ALasso* GetLassoWeapon() const;
};
