// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponReload.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "CattleGame/Weapons/Revolver/Revolver.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "CattleGame/CattleGame.h"

UGA_WeaponReload::UGA_WeaponReload()
{
	// Configure the ability
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// Reload must be authoritative to keep ammo in sync; run only on server
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// Set the ability tags
	FGameplayTagContainer Tags;
	Tags.AddTag(CattleGameplayTags::Ability_Weapon_Reload);
	SetAssetTags(Tags);

	// This ability is blocked while firing
	BlockAbilitiesWithTag.AddTag(CattleGameplayTags::State_Weapon_Firing);

	// This ability cannot be activated while already reloading
	CancelAbilitiesWithTag.AddTag(CattleGameplayTags::State_Weapon_Reloading);

	// Fire is blocked while reloading
	ActivationBlockedTags.AddTag(CattleGameplayTags::Ability_Weapon_Fire);
}

void UGA_WeaponReload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Warning, TEXT("Reload Ability: ActivateAbility called (Auth=%s, Owner=%s)"),
		   ActorInfo && ActorInfo->IsNetAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		   ActorInfo && ActorInfo->OwnerActor.IsValid() ? *ActorInfo->OwnerActor->GetName() : TEXT("<null>"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Start reloading
	StartReload();
}

bool UGA_WeaponReload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags, OUT FGameplayTagContainer *OptionalRelevantTags) const
{
	// First check base ability activation
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Reload Ability: CanActivateAbility BLOCKED - Base ability check failed"));
		return false;
	}

	// Check if weapon exists (use ActorInfo because caches may not be set yet here)
	AWeaponBase *Weapon = ResolveWeapon(ActorInfo);
	if (!Weapon)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Reload Ability: CanActivateAbility BLOCKED - No weapon equipped"));
		return false;
	}

	// Check if weapon can reload
	// For Revolver specifically
	if (ARevolver *Revolver = Cast<ARevolver>(Weapon))
	{
		bool bCanReload = Revolver->CanReload();
		UE_LOG(LogGASDebug, Warning, TEXT("Reload Ability: CanActivateAbility - Revolver->CanReload() = %d, ammo: %d/%d, bIsReloading: %s"),
			   bCanReload, Revolver->CurrentAmmo, Revolver->MaxAmmo,
			   Revolver->bIsReloading ? TEXT("TRUE") : TEXT("FALSE"));
		return bCanReload;
	}

	// For other weapons, just check if they exist
	UE_LOG(LogGASDebug, Warning, TEXT("Reload Ability: CanActivateAbility - Non-Revolver weapon, allowing reload"));
	return true;
}

void UGA_WeaponReload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Clear the reload timer if it's set
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	// If cancelled, call the cancelled callback
	if (bWasCancelled)
	{
		OnReloadCancelled();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_WeaponReload::OnReloadComplete()
{
	UE_LOG(LogGASDebug, Warning, TEXT("OnReloadComplete: Reload finished"));

	AWeaponBase *Weapon = GetWeapon_Implementation();
	if (!Weapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("OnReloadComplete: Failed to get weapon"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// Revolver-specific completion
	if (ARevolver *Revolver = Cast<ARevolver>(Weapon))
	{
		const bool bAuth = (CurrentActorInfo && CurrentActorInfo->IsNetAuthority());
		UE_LOG(LogGASDebug, Warning, TEXT("OnReloadComplete: Calling Revolver->OnReloadComplete() on %s, ammo before: %d/%d"),
			   bAuth ? TEXT("SERVER") : TEXT("CLIENT"), Revolver->CurrentAmmo, Revolver->MaxAmmo);
		Revolver->OnReloadComplete();
		UE_LOG(LogGASDebug, Warning, TEXT("OnReloadComplete: Revolver ammo after reload: %d/%d"),
			   Revolver->CurrentAmmo, Revolver->MaxAmmo);
	}

	// Remove reloading tag
	if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(CattleGameplayTags::State_Weapon_Reloading);
	}

	// End the ability
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_WeaponReload::OnReloadCancelled()
{
	UE_LOG(LogGASDebug, Warning, TEXT("OnReloadCancelled: Reload was cancelled"));

	// Remove reloading tag when cancelled
	if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(CattleGameplayTags::State_Weapon_Reloading);
	}
}

void UGA_WeaponReload::StartReload()
{
	AWeaponBase *Weapon = GetWeapon_Implementation();
	if (!Weapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("StartReload: Failed to get weapon"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("StartReload: Beginning reload for weapon %s"), *Weapon->GetName());

	// Add reloading state tag
	if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(CattleGameplayTags::State_Weapon_Reloading);
	}

	// Broadcast that reload is starting
	OnReloadStarted(Weapon);

	// Call the weapon's reload method
	Weapon->Reload();

	// Get reload duration from weapon
	float ReloadDuration = 2.0f; // Default fallback

	if (ARevolver *Revolver = Cast<ARevolver>(Weapon))
	{
		ReloadDuration = Revolver->ReloadTime;
		UE_LOG(LogGASDebug, Warning, TEXT("StartReload: Revolver reload duration = %.2f seconds"), ReloadDuration);
	}

	// Set timer for reload completion
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&UGA_WeaponReload::OnReloadComplete,
			ReloadDuration,
			false);
		UE_LOG(LogGASDebug, Warning, TEXT("StartReload: Reload timer set for %.2f seconds"), ReloadDuration);
	}
}
