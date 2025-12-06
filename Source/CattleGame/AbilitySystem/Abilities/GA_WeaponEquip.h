// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_WeaponEquip.generated.h"

class AWeaponBase;
class ACattleCharacter;
class USkeletalMeshComponent;

/**
 * UGA_WeaponEquip
 *
 * Gameplay ability for equipping/unequipping weapons.
 * Handles weapon attachment, mesh visibility, and cosmetic effects.
 *
 * This ability is triggered by the InventoryComponent when a weapon slot is equipped.
 */
UCLASS()
class CATTLEGAME_API UGA_WeaponEquip : public UGA_Weapon
{
    GENERATED_BODY()

public:
    UGA_WeaponEquip();

    /**
     * Set whether this is an equip (true) or unequip (false) operation.
     * Called by InventoryComponent before activating the ability.
     */
    UFUNCTION(BlueprintCallable, Category = "Weapon Equip")
    void SetIsEquipping(bool bEquip);

    /**
     * Set the target weapon for equip/unequip.
     * Called by InventoryComponent before activating the ability.
     */
    UFUNCTION(BlueprintCallable, Category = "Weapon Equip")
    void SetTargetWeapon(AWeaponBase *Weapon);

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

    /**
     * Called when a weapon is equipped. Override in Blueprint to handle cosmetic effects.
     * Default C++ implementation handles attachment and visibility.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Weapon Equip")
    void OnEquipWeapon(AWeaponBase *Weapon, ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

    /**
     * Called when a weapon is unequipped. Override in Blueprint to handle cosmetic effects.
     * Default C++ implementation handles detachment and hiding.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Weapon Equip")
    void OnUnequipWeapon(AWeaponBase *Weapon, ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

private:
    /** Whether this activation is an equip (true) or unequip (false) */
    bool bIsEquipping = true;

    /** Target weapon for this activation */
    UPROPERTY()
    AWeaponBase *TargetWeapon = nullptr;
};
