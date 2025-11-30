// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "CattleAttributeSet.generated.h"

/**
 * FCattleAttributeSet
 *
 * Core attributes for the Cattle Game character system.
 * Includes health and movement speed attributes with proper replication support.
 */
UCLASS()
class CATTLEGAME_API UCattleAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UCattleAttributeSet();

	// ===== Health Attributes =====

	/** Current health of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;

	/** Maximum health of the character */
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;

	// ===== Movement Attributes =====

	/** Current movement speed multiplier (1.0 = 100% speed) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement", ReplicatedUsing = OnRep_MovementSpeed)
	FGameplayAttributeData MovementSpeed;

	// ===== Damage Meta Attribute =====

	/** Meta attribute used to apply incoming damage (not replicated, server-side only) */
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData Damage;

	// Manual Attribute Accessors (since ATTRIBUTE_ACCESSORS macro has issues)
	static FGameplayAttribute GetHealthAttribute();
	static FGameplayAttribute GetMaxHealthAttribute();
	static FGameplayAttribute GetMovementSpeedAttribute();
	static FGameplayAttribute GetDamageAttribute();

	float GetHealth() const { return Health.GetCurrentValue(); }
	void SetHealth(float NewValue)
	{
		Health.SetCurrentValue(NewValue);
		Health.SetBaseValue(NewValue);
	}

	float GetMaxHealth() const { return MaxHealth.GetCurrentValue(); }
	void SetMaxHealth(float NewValue)
	{
		MaxHealth.SetCurrentValue(NewValue);
		MaxHealth.SetBaseValue(NewValue);
	}

	float GetMovementSpeed() const { return MovementSpeed.GetCurrentValue(); }
	void SetMovementSpeed(float NewValue)
	{
		MovementSpeed.SetCurrentValue(NewValue);
		MovementSpeed.SetBaseValue(NewValue);
	}

	float GetDamage() const { return Damage.GetCurrentValue(); }
	void SetDamage(float NewValue)
	{
		Damage.SetCurrentValue(NewValue);
		Damage.SetBaseValue(NewValue);
	}

	// Replication callbacks
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData &OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData &OldValue);

	UFUNCTION()
	void OnRep_MovementSpeed(const FGameplayAttributeData &OldValue);

	// Attribute System Overrides
	virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	// Helper function to apply damage
	void TakeDamage(float DamageAmount);
};
