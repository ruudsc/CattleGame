// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "CattleAbilitySystemComponent.generated.h"

class UGameplayAbility;
class UCattleAttributeSet;

/**
 * FCattleAbilitySystemComponent
 *
 * Custom ability system component for Cattle Game with project-specific functionality.
 * Handles ability initialization, granting, and networking.
 */
UCLASS(ClassGroup = (AbilitySystem), meta = (BlueprintSpawnableComponent))
class CATTLEGAME_API UCattleAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UCattleAbilitySystemComponent();

	virtual void BeginPlay() override;

	/**
	 * Initialize the ability system for this character.
	 * Must be called after the character is possessed by a controller.
	 *
	 * @param InOwnerActor The actor that owns this ability system
	 * @param InAvatarActor The actor that represents the avatar (usually the character itself)
	 */
	virtual void InitializeAbilitySystem(AActor *InOwnerActor, AActor *InAvatarActor);

	/**
	 * Grant an ability to this character.
	 *
	 * @param AbilityClass The gameplay ability class to grant
	 * @param Level The ability level
	 * @param InputID The input ID for this ability (optional)
	 * @return Handle to the granted ability
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FGameplayAbilitySpecHandle GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 1, int32 InputID = -1);

	/**
	 * Remove an ability from this character.
	 *
	 * @param AbilitySpecHandle The handle to the ability to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveAbility(FGameplayAbilitySpecHandle AbilitySpecHandle);

	/**
	 * Activate an ability by tag.
	 *
	 * @param AbilityTag The tag identifying the ability to activate
	 * @return True if the ability was activated
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool ActivateAbilityByTag(const FGameplayTag &AbilityTag);

	/**
	 * Get the AttributeSet for this character.
	 */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	UCattleAttributeSet *GetCattleAttributeSet() const { return AttributeSet; }

protected:
	/** The AttributeSet for this character */
	UPROPERTY()
	UCattleAttributeSet *AttributeSet;

	/**
	 * Called when the character's health changes.
	 * Override in blueprints to respond to health changes.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Attributes")
	void OnHealthChanged(float NewHealth, float MaxHealth);

	/**
	 * Called when the character dies (health reaches 0).
	 * Override in blueprints to handle death.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Attributes")
	void OnDeath();

private:
	// Ability-related callbacks
	void OnHealthChangedCallback(const FOnAttributeChangeData &Data);
};
