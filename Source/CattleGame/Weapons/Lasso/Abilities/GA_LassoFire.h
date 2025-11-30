#pragma once

#include "CoreMinimal.h"
#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_LassoFire.generated.h"

class ALasso;

/**
 * Lasso Fire Ability - Cast lasso projectile.
 *
 * Input Binding: Primary Fire (Mouse LMB)
 * Activation: Started (on key press)
 * Duration: While held, maintains lasso pull force
 * Cleanup: OnCompleted, releases lasso if active
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_LassoFire : public UGA_Weapon
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									const FGameplayTagContainer *SourceTags = nullptr, const FGameplayTagContainer *TargetTags = nullptr,
									OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;

protected:
	/** Get lasso weapon (cast to ALasso) */
	ALasso *GetLassoWeapon() const;

	/** Fire the lasso projectile */
	void FireLasso();
};
