// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "CattleGameplayTags.h"

UCattleAttributeSet::UCattleAttributeSet()
{
	// Initialize attribute values
	Health.SetBaseValue(100.0f);
	Health.SetCurrentValue(100.0f);

	MaxHealth.SetBaseValue(100.0f);
	MaxHealth.SetCurrentValue(100.0f);

	MovementSpeed.SetBaseValue(1.0f);
	MovementSpeed.SetCurrentValue(1.0f);

	Damage.SetBaseValue(0.0f);
	Damage.SetCurrentValue(0.0f);
}

// Static attribute accessor implementations
FGameplayAttribute UCattleAttributeSet::GetHealthAttribute()
{
	static FProperty *Property = FindFieldChecked<FProperty>(UCattleAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UCattleAttributeSet, Health));
	return FGameplayAttribute(Property);
}

FGameplayAttribute UCattleAttributeSet::GetMaxHealthAttribute()
{
	static FProperty *Property = FindFieldChecked<FProperty>(UCattleAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UCattleAttributeSet, MaxHealth));
	return FGameplayAttribute(Property);
}

FGameplayAttribute UCattleAttributeSet::GetMovementSpeedAttribute()
{
	static FProperty *Property = FindFieldChecked<FProperty>(UCattleAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UCattleAttributeSet, MovementSpeed));
	return FGameplayAttribute(Property);
}

FGameplayAttribute UCattleAttributeSet::GetDamageAttribute()
{
	static FProperty *Property = FindFieldChecked<FProperty>(UCattleAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UCattleAttributeSet, Damage));
	return FGameplayAttribute(Property);
}

void UCattleAttributeSet::OnRep_Health(const FGameplayAttributeData &OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCattleAttributeSet, Health, OldValue);
}

void UCattleAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData &OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCattleAttributeSet, MaxHealth, OldValue);
}

void UCattleAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData &OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCattleAttributeSet, MovementSpeed, OldValue);
}

void UCattleAttributeSet::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamp Health
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}

	// Clamp MaxHealth
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}

	// Clamp MovementSpeed
	if (Attribute == GetMovementSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 5.0f); // 0-500% speed
	}
}

void UCattleAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Handle damage being applied
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Only apply damage on the server
		UAbilitySystemComponent *OwningASC = GetOwningAbilitySystemComponent();
		AActor *OwnerActor = OwningASC ? OwningASC->GetOwner() : nullptr;
		if (OwnerActor && OwnerActor->HasAuthority())
		{
			// Store the damage amount
			float DamageAmount = GetDamage();

			// Clear the damage meta attribute
			SetDamage(0.0f);

			// Apply damage to health
			if (DamageAmount > 0.0f)
			{
				float OldHealth = Health.GetBaseValue();
				SetHealth(FMath::Max(0.0f, GetHealth() - DamageAmount));

				if (GetHealth() <= 0.0f && OldHealth > 0.0f)
				{
					// Character died
					if (UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent())
					{
						ASC->AddLooseGameplayTag(CattleGameplayTags::State_Dead);
					}
				}
			}
		}
	}

	// Handle MaxHealth changes
	if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// If health is higher than max health, clamp it
		if (GetHealth() > GetMaxHealth())
		{
			UAbilitySystemComponent *OwningASC = GetOwningAbilitySystemComponent();
			AActor *OwnerActor = OwningASC ? OwningASC->GetOwner() : nullptr;
			if (OwnerActor && OwnerActor->HasAuthority())
			{
				SetHealth(GetMaxHealth());
			}
		}
	}
}

void UCattleAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate health attributes to all clients
	DOREPLIFETIME_CONDITION_NOTIFY(UCattleAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCattleAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCattleAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
}

void UCattleAttributeSet::TakeDamage(float DamageAmount)
{
	if (DamageAmount > 0.0f)
	{
		SetDamage(DamageAmount);
	}
}
