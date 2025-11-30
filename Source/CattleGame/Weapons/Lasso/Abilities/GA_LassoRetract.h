#pragma once

#include "CoreMinimal.h"
#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_LassoRetract.generated.h"

class ALasso;

/**
 * Lasso Release/Retract Ability - Release tether or retract lasso on secondary fire.
 *
 * Input Binding: Secondary Fire (Mouse RMB)
 * Activation: Started (on key press)
 * Effect: Releases any tethered target and starts retracting the lasso
 * Duration: Instant (ability ends immediately, retraction happens in weapon/projectile)
 *
 * Handles two scenarios:
 * 1. Mid-flight retraction: Lasso in flight, player wants to cancel throw
 * 2. Release tethered target: Lasso caught target, player releases to let go
 *
 * In both cases, the projectile begins moving back toward the player.
 * The lasso will have a 0.5s cooldown before it can be thrown again.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_LassoRetract : public UGA_Weapon
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									const FGameplayTagContainer *SourceTags = nullptr, const FGameplayTagContainer *TargetTags = nullptr,
									OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;

protected:
	/** Get lasso weapon (cast to ALasso) */
	ALasso *GetLassoWeapon() const;

	/** Release tether and/or start retracting the lasso */
	void ReleaseLasso();
};
