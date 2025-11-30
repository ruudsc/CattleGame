// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_SwitchWeapon.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "InputActionValue.h"

UGA_SwitchWeapon::UGA_SwitchWeapon()
	: LastSwitchTime(-9999.0f), SwitchCooldown(0.2f) // 200ms cooldown between switches
{
	// Never cancel on release
	bCancelAbilityOnInputReleased = false;
}

void UGA_SwitchWeapon::OnTriggeredInputAction_Implementation(const FInputActionValue &Value)
{
	// Get the axis value (scroll wheel delta)
	const float AxisValue = Value.Get<float>();

	// Ignore very small values (noise)
	if (FMath::Abs(AxisValue) < 0.1f)
	{
		return;
	}

	// Get the character
	ACattleCharacter *Character = Cast<ACattleCharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	// Check cooldown to prevent rapid switching
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastSwitchTime < SwitchCooldown)
	{
		return;
	}

	LastSwitchTime = CurrentTime;

	// Get inventory component
	UInventoryComponent *Inventory = Character->GetInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	// Switch weapon based on scroll direction
	if (AxisValue > 0.0f)
	{
		// Scroll up = next weapon
		Inventory->CycleToNextWeapon();
	}
	else if (AxisValue < 0.0f)
	{
		// Scroll down = previous weapon
		Inventory->CycleToPreviousWeapon();
	}
}
