#pragma once

#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_TrumpetScare.generated.h"

class ATrumpet;

/**
 * Trumpet Scare Ability - Play scary note to repel enemies.
 *
 * Input Binding: Secondary Fire (Mouse RMB)
 * Activation: Started (on key press)
 * Duration: While held, applies scare effect to nearby enemies
 * Cleanup: OnCompleted, stops trumpet
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_TrumpetScare : public UGA_Weapon
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
	/** Get trumpet weapon (cast to ATrumpet) */
	ATrumpet *GetTrumpetWeapon() const;

	/** Start playing scare */
	void PlayScare();
};
