// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponEquip.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "CattleGame/Weapons/Lasso/Lasso.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "CattleGame/CattleGame.h"

UGA_WeaponEquip::UGA_WeaponEquip()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // Set the ability tags
    FGameplayTagContainer Tags;
    Tags.AddTag(CattleGameplayTags::Ability_Weapon_Equip);
    SetAssetTags(Tags);
}

void UGA_WeaponEquip::SetIsEquipping(bool bEquip)
{
    bIsEquipping = bEquip;
}

void UGA_WeaponEquip::SetTargetWeapon(AWeaponBase *Weapon)
{
    TargetWeapon = Weapon;
}

void UGA_WeaponEquip::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(LogGASDebug, Warning, TEXT("GA_WeaponEquip::ActivateAbility - TargetWeapon=%s, bIsEquipping=%s"),
           TargetWeapon ? *TargetWeapon->GetName() : TEXT("NULL"), bIsEquipping ? TEXT("TRUE") : TEXT("FALSE"));

    ACattleCharacter *Character = GetCharacterOwner();
    if (!Character || !TargetWeapon)
    {
        UE_LOG(LogGASDebug, Error, TEXT("GA_WeaponEquip: BLOCKED - No character (%p) or weapon (%p)"), Character, TargetWeapon);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    USkeletalMeshComponent *ActiveMesh = Character->GetActiveCharacterMesh();

    if (bIsEquipping)
    {
        UE_LOG(LogGASDebug, Warning, TEXT("GA_WeaponEquip: Equipping weapon %s"), *TargetWeapon->GetName());
        OnEquipWeapon(TargetWeapon, Character, ActiveMesh);
    }
    else
    {
        UE_LOG(LogGASDebug, Warning, TEXT("GA_WeaponEquip: Unequipping weapon %s"), *TargetWeapon->GetName());
        OnUnequipWeapon(TargetWeapon, Character, ActiveMesh);
    }

    // Ability completes immediately
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_WeaponEquip::OnEquipWeapon_Implementation(AWeaponBase *Weapon, ACattleCharacter *Character, USkeletalMeshComponent *Mesh)
{
    if (!Weapon || !Character || !Mesh)
    {
        UE_LOG(LogGASDebug, Error, TEXT("GA_WeaponEquip::OnEquipWeapon - Missing Weapon=%p, Character=%p, Mesh=%p"),
               Weapon, Character, Mesh);
        return;
    }

    // Mark weapon as equipped
    Weapon->bIsEquipped = true;

    // Attach to character's hand if not already attached
    if (Weapon->GetAttachParentActor() != Character)
    {
        Weapon->AttachToCharacterHand();
    }

    // Show the weapon
    Weapon->SetActorHiddenInGame(false);

    UE_LOG(LogGASDebug, Warning, TEXT("GA_WeaponEquip::OnEquipWeapon - EQUIPPED weapon %s, Hidden=%s"),
           *Weapon->GetName(), Weapon->IsHidden() ? TEXT("TRUE") : TEXT("FALSE"));
}

void UGA_WeaponEquip::OnUnequipWeapon_Implementation(AWeaponBase *Weapon, ACattleCharacter *Character, USkeletalMeshComponent *Mesh)
{
    if (!Weapon)
    {
        UE_LOG(LogGASDebug, Error, TEXT("GA_WeaponEquip::OnUnequipWeapon - No weapon!"));
        return;
    }

    // If this is a Lasso, force reset to stop any ongoing state machine tick
    if (ALasso* Lasso = Cast<ALasso>(Weapon))
    {
        UE_LOG(LogGASDebug, Log, TEXT("GA_WeaponEquip::OnUnequipWeapon - ForceReset on Lasso"));
        Lasso->ForceReset();
    }

    // Mark weapon as not equipped
    Weapon->bIsEquipped = false;

    // Hide the weapon - no need to detach, just hide it
    // Keeping it attached avoids transform issues when re-equipping
    Weapon->SetActorHiddenInGame(true);

    UE_LOG(LogGASDebug, Warning, TEXT("GA_WeaponEquip::OnUnequipWeapon - UNEQUIPPED weapon %s, Hidden=%s"),
           *Weapon->GetName(), Weapon->IsHidden() ? TEXT("TRUE") : TEXT("FALSE"));
}
