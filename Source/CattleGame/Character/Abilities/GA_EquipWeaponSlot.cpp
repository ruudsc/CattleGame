// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_EquipWeaponSlot.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"

UGA_EquipWeaponSlot::UGA_EquipWeaponSlot()
	: SlotIndexToEquip(0)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_EquipWeaponSlot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("EquipWeaponSlot: ActivateAbility called for slot %d"), SlotIndexToEquip);

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Get the character
	ACattleCharacter *Character = Cast<ACattleCharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Get inventory component
	UInventoryComponent *Inventory = Character->GetInventoryComponent();
	if (!Inventory)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Equip the specified weapon slot
	Inventory->EquipWeapon(SlotIndexToEquip);

	// End the ability immediately (instant action)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
