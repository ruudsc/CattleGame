#pragma once

#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_WeaponReload.generated.h"

/**
 * UGA_WeaponReload
 *
 * Gameplay ability for reloading the equipped weapon.
 * Handles reload input and weapon reload animations through GAS.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_WeaponReload : public UGA_Weapon
{
	GENERATED_BODY()

public:
	UGA_WeaponReload();

	/**
	 * Called when ability is activated
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

	/**
	 * Can the ability be activated?
	 * Checks if weapon exists and can reload
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayTagContainer *SourceTags = nullptr, const FGameplayTagContainer *TargetTags = nullptr, OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;

	/**
	 * Called when reload ability ends
	 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * When reload completes, this is called
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload Ability")
	void OnReloadComplete();

	/**
	 * When reload is cancelled or interrupted
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload Ability")
	void OnReloadCancelled();

protected:
	/**
	 * Called when reload ability starts
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Reload Ability")
	void OnReloadStarted(AWeaponBase *Weapon);

	/**
	 * Start reloading the weapon
	 */
	void StartReload();

	/** Handle for reload timer */
	FTimerHandle ReloadTimerHandle;
};
