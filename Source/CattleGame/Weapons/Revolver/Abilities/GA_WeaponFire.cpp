// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponFire.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "CattleGame/Weapons/Revolver/Revolver.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Camera/CameraComponent.h"
#include "CattleGame/CattleGame.h"

UGA_WeaponFire::UGA_WeaponFire()
{
	// Configure the ability
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Set the ability tags
	FGameplayTagContainer Tags;
	Tags.AddTag(CattleGameplayTags::Ability_Weapon_Fire);
	SetAssetTags(Tags);

	// This ability is blocked while reloading
	BlockAbilitiesWithTag.AddTag(CattleGameplayTags::State_Weapon_Reloading);

	// This ability cannot be activated while firing (server-side only)
	CancelAbilitiesWithTag.AddTag(CattleGameplayTags::State_Weapon_Firing);
}

void UGA_WeaponFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: ActivateAbility called"));
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Fire the weapon
	UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: Calling FireWeapon()"));
	FireWeapon();
	UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: FireWeapon() completed"));
}

bool UGA_WeaponFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags, OUT FGameplayTagContainer *OptionalRelevantTags) const
{
	// First check base ability activation
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		UE_LOG(LogGASDebug, Error, TEXT("Fire Ability: BLOCKED - Super::CanActivateAbility returned false"));
		return false;
	}

	// Resolve equipped weapon using the provided ActorInfo (since caches may not be set yet)
	AWeaponBase *Weapon = ResolveWeapon(ActorInfo);
	if (!Weapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("Fire Ability: BLOCKED - No weapon equipped"));
		return false;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: Weapon found at Addr: %p, checking CanFire()"), Weapon);

	// Check if weapon can fire
	// For Revolver specifically
	if (ARevolver *Revolver = Cast<ARevolver>(Weapon))
	{
		bool bCanFire = Revolver->CanFire();
		UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: Revolver->CanFire() = %d"), bCanFire);
		return bCanFire;
	}

	// For other weapons, just check if they exist
	return true;
}

void UGA_WeaponFire::FireWeapon()
{
	UE_LOG(LogGASDebug, Warning, TEXT("FireWeapon: Getting weapon"));
	AWeaponBase *Weapon = GetWeapon_Implementation();
	if (!Weapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("FireWeapon: Weapon is NULL, ending ability"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("FireWeapon: Got weapon %p, getting character"), Weapon);
	ACattleCharacter *Character = GetCharacterOwner();
	if (!Character)
	{
		UE_LOG(LogGASDebug, Error, TEXT("FireWeapon: Character is NULL, ending ability"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("FireWeapon: Got character, adding firing tag"));

	// Get trace start/direction from character's camera
	UCameraComponent *Camera = Character->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		UE_LOG(LogGASDebug, Error, TEXT("FireWeapon: No camera component found"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FVector TraceStart = Camera->GetComponentLocation();
	FVector TraceDir = Camera->GetForwardVector();

	// Add firing state tag and trigger muzzle flash cue with location data
	if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(CattleGameplayTags::State_Weapon_Firing);

		// Set up cue parameters with muzzle location
		FGameplayCueParameters MuzzleFlashParams;
		MuzzleFlashParams.Location = TraceStart;
		MuzzleFlashParams.Normal = TraceDir;
		MuzzleFlashParams.SourceObject = Weapon;
		MuzzleFlashParams.Instigator = Character;

		ASC->ExecuteGameplayCue(CattleGameplayTags::GameplayCue_Revolver_Fire, MuzzleFlashParams);
	}

	// Broadcast that fire is starting
	UE_LOG(LogGASDebug, Warning, TEXT("FireWeapon: Broadcasting OnFireStarted"));
	OnFireStarted(Weapon);

	// Call the weapon's server fire to do the actual shot (damage + impact cue)
	if (ARevolver *Revolver = Cast<ARevolver>(Weapon))
	{
		UE_LOG(LogGASDebug, Warning, TEXT("FireWeapon: Calling Revolver->RequestServerFireWithPrediction"));
		Revolver->RequestServerFireWithPrediction(TraceStart, TraceDir);
	}

	// Remove firing state tag
	if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(CattleGameplayTags::State_Weapon_Firing);
	}

	// End the ability
	UE_LOG(LogGASDebug, Warning, TEXT("FireWeapon: Ending ability"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
