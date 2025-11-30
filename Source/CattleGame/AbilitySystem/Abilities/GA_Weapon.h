// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_Weapon.generated.h"

class AWeaponBase;
class ACattleCharacter;

/**
 * UGA_Weapon
 *
 * Base class for all weapon-related gameplay abilities (Fire, Reload, etc.).
 * Provides common functionality for interacting with weapons.
 */
UCLASS()
class CATTLEGAME_API UGA_Weapon : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Weapon();

	/**
	 * Get the weapon associated with this ability.
	 * This must be overridden or set in blueprints.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon Ability")
	AWeaponBase *GetWeapon() const;

	/**
	 * Get the character that owns this ability.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Ability")
	ACattleCharacter *GetCharacterOwner() const;

	/**
	 * Called when the ability ends
	 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/**
	 * Called when ability starts
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

	/**
	 * Broadcast ability activation (for gameplay cues, animations, etc.)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Ability")
	void OnAbilityActivated();

	/**
	 * Broadcast ability end
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Ability")
	void OnAbilityEnded();

	/** Reference to character owner for caching */
	UPROPERTY()
	ACattleCharacter *CachedCharacterOwner;

	/** Reference to weapon for caching */
	UPROPERTY()
	AWeaponBase *CachedWeapon;

protected:
	/**
	 * Resolve the owning character using the most reliable source available.
	 * Prefer the cached owner (set during ActivateAbility), but fall back to
	 * the provided ActorInfo when queried during CanActivateAbility.
	 */
	ACattleCharacter *ResolveCharacterOwner(const FGameplayAbilityActorInfo *ActorInfo) const;

	/**
	 * Resolve the currently equipped weapon for the owning character.
	 * Uses ResolveCharacterOwner to obtain the character, then queries their inventory.
	 */
	AWeaponBase *ResolveWeapon(const FGameplayAbilityActorInfo *ActorInfo) const;
};
