// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_DynamiteThrow.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/Weapons/Dynamite/Dynamite.h"
#include "CattleGame/CattleGame.h"

UGA_DynamiteThrow::UGA_DynamiteThrow()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_DynamiteThrow::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(LogGASDebug, Log, TEXT("GA_DynamiteThrow: ActivateAbility called"));

    // Throw the dynamite
    ThrowDynamite();
}

bool UGA_DynamiteThrow::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags, OUT FGameplayTagContainer *OptionalRelevantTags) const
{
    // First check base ability activation
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        UE_LOG(LogGASDebug, Warning, TEXT("GA_DynamiteThrow: BLOCKED - Super::CanActivateAbility returned false"));
        return false;
    }

    // Resolve equipped weapon using the provided ActorInfo
    AWeaponBase *Weapon = ResolveWeapon(ActorInfo);
    if (!Weapon)
    {
        UE_LOG(LogGASDebug, Warning, TEXT("GA_DynamiteThrow: BLOCKED - No weapon equipped"));
        return false;
    }

    // Check if it's dynamite and can fire
    ADynamite *Dynamite = Cast<ADynamite>(Weapon);
    if (!Dynamite)
    {
        UE_LOG(LogGASDebug, Warning, TEXT("GA_DynamiteThrow: BLOCKED - Equipped weapon is not Dynamite"));
        return false;
    }

    if (!Dynamite->CanFire())
    {
        UE_LOG(LogGASDebug, Warning, TEXT("GA_DynamiteThrow: BLOCKED - Dynamite cannot fire (no ammo?)"));
        return false;
    }

    return true;
}

ADynamite *UGA_DynamiteThrow::GetDynamiteWeapon() const
{
    return Cast<ADynamite>(GetWeapon());
}

void UGA_DynamiteThrow::ThrowDynamite()
{
    ADynamite *Dynamite = GetDynamiteWeapon();
    if (!Dynamite)
    {
        UE_LOG(LogGASDebug, Error, TEXT("GA_DynamiteThrow::ThrowDynamite - No dynamite weapon found"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // Fire blueprint event
    OnThrowStarted(Dynamite);

    // Call the weapon's Fire method (handles projectile spawning)
    Dynamite->Fire();

    UE_LOG(LogGASDebug, Log, TEXT("GA_DynamiteThrow::ThrowDynamite - Dynamite thrown"));

    // End ability
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
