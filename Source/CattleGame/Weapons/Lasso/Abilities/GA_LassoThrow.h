#pragma once

#include "CoreMinimal.h"
#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_LassoThrow.generated.h"

class ALasso;

/**
 * Lasso Throw Ability - Main lasso action.
 *
 * Behavior:
 * - Idle state: Throw lasso (spawn projectile), ends immediately
 * - Tethered state: Pull target while input held, ends on release
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_LassoThrow : public UGA_Weapon
{
	GENERATED_BODY()

public:
	UGA_LassoThrow();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									const FGameplayTagContainer *SourceTags = nullptr, const FGameplayTagContainer *TargetTags = nullptr,
									OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;

	/** Called when input is released - used to stop pulling */
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	/** Get lasso weapon (cast) */
	ALasso *GetLassoWeapon() const;

	/** Execute the throw */
	void ExecuteThrow();

	/** Track if we're in pull mode (need to end ability on input release) */
	bool bIsPullMode = false;
};
