// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

UGA_Weapon::UGA_Weapon()
	: CachedCharacterOwner(nullptr), CachedWeapon(nullptr)
{
	// Weapon abilities are typically instant or very short duration
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Weapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Cache the character owner
	if (ActorInfo && ActorInfo->OwnerActor.IsValid())
	{
		CachedCharacterOwner = Cast<ACattleCharacter>(ActorInfo->OwnerActor.Get());
		UE_LOG(LogGASDebug, Log, TEXT("GA_Weapon: CachedCharacterOwner set to %s"), CachedCharacterOwner ? TEXT("VALID") : TEXT("NULL"));
	}
	else
	{
		UE_LOG(LogGASDebug, Error, TEXT("GA_Weapon: ActivateAbility - ActorInfo or OwnerActor is invalid!"));
	}

	// Broadcast activation
	OnAbilityActivated();
}

void UGA_Weapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Broadcast end
	OnAbilityEnded();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AWeaponBase *UGA_Weapon::GetWeapon_Implementation() const
{
	AWeaponBase *Weapon = ResolveWeapon(CurrentActorInfo);
	if (!Weapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("GetWeapon_Implementation: Failed to resolve equipped weapon"));
	}
	return Weapon;
}

ACattleCharacter *UGA_Weapon::GetCharacterOwner() const
{
	// Try cached character owner first
	if (CachedCharacterOwner)
	{
		return CachedCharacterOwner;
	}

	// If cached owner is NULL, try to get it from the ability's actor info
	if (CurrentActorInfo)
	{
		ACattleCharacter *CharacterOwner = Cast<ACattleCharacter>(CurrentActorInfo->OwnerActor.Get());
		if (CharacterOwner)
		{
			return CharacterOwner;
		}
	}

	return nullptr;
}

ACattleCharacter *UGA_Weapon::ResolveCharacterOwner(const FGameplayAbilityActorInfo *ActorInfo) const
{
	// Prefer cached owner (once ability is active)
	if (CachedCharacterOwner)
	{
		return CachedCharacterOwner;
	}

	// During CanActivateAbility, rely on the ActorInfo provided by the query
	if (ActorInfo && ActorInfo->OwnerActor.IsValid())
	{
		return Cast<ACattleCharacter>(ActorInfo->OwnerActor.Get());
	}

	// As a final fallback, try CurrentActorInfo if available
	if (CurrentActorInfo && CurrentActorInfo->OwnerActor.IsValid())
	{
		return Cast<ACattleCharacter>(CurrentActorInfo->OwnerActor.Get());
	}

	return nullptr;
}

AWeaponBase *UGA_Weapon::ResolveWeapon(const FGameplayAbilityActorInfo *ActorInfo) const
{
	ACattleCharacter *CharacterOwner = ResolveCharacterOwner(ActorInfo);
	if (!CharacterOwner)
	{
		UE_LOG(LogGASDebug, Error, TEXT("ResolveWeapon: CharacterOwner could not be resolved (cached and ActorInfo both null)"));
		return nullptr;
	}

	if (UInventoryComponent *Inventory = CharacterOwner->GetInventoryComponent())
	{
		AWeaponBase *EquippedWeapon = Inventory->GetEquippedWeapon();
		const bool bAuth = (ActorInfo && ActorInfo->IsNetAuthority());
		UE_LOG(LogGASDebug, Log, TEXT("ResolveWeapon [%s]: EquippedWeapon=%p (%s) for Owner=%s"),
			   bAuth ? TEXT("SERVER") : TEXT("CLIENT"),
			   EquippedWeapon,
			   EquippedWeapon ? *EquippedWeapon->GetName() : TEXT("<null>"),
			   *CharacterOwner->GetName());
		return EquippedWeapon;
	}

	UE_LOG(LogGASDebug, Error, TEXT("ResolveWeapon: Inventory component is NULL (Owner=%s)"), *GetNameSafe(CharacterOwner));
	return nullptr;
}
