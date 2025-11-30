#pragma once

#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_WeaponFire.generated.h"

/**
 * UGA_WeaponFire
 *
 * Gameplay ability for firing the equipped weapon.
 * Handles fire input activation and weapon firing through GAS.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_WeaponFire : public UGA_Weapon
{
	GENERATED_BODY()

public:
	UGA_WeaponFire();

	/**
	 * Called when ability is activated
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

	/**
	 * Can the ability be activated?
	 * Checks if weapon exists and can fire
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayTagContainer *SourceTags = nullptr, const FGameplayTagContainer *TargetTags = nullptr, OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;

	/**
	 * Cost of firing the weapon
	 * Can be configured in blueprints (e.g., ammo cost)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fire Ability|Cost")
	float FireCost = 1.0f;

	/**
	 * Cooldown between shots in seconds
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fire Ability|Cooldown")
	float FireCooldown = 0.5f;

protected:
	/**
	 * Apply fire cost (ammo, stamina, etc.)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Fire Ability")
	void ApplyFireCost();

	/**
	 * Called when fire ability starts
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Fire Ability")
	void OnFireStarted(AWeaponBase *Weapon);

	/**
	 * Fire the weapon and add gameplay tags to indicate firing state
	 */
	void FireWeapon();
};
