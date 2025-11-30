#pragma once

#include "CoreMinimal.h"
#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_LassoPullPawnIn.generated.h"

class ALasso;

/**
 * Lasso Pull Ability - Hold primary fire after catching a target to pull them in.
 *
 * Input Binding: Primary Fire (Mouse LMB)
 * Activation: Started (on key press, ONLY if lasso is already caught)
 * Duration: While held, maintains pull force on caught target
 * Cleanup: OnCompleted (fire button released)
 *
 * This ability activates ONLY when the lasso has already caught a target.
 * It allows the player to hold primary fire to continuously pull the target in.
 * Does NOT fire a new lasso - just maintains the pull.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_LassoPullPawnIn : public UGA_Weapon
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
};
