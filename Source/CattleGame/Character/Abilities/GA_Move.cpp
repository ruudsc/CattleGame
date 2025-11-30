// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Move.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "InputActionValue.h"

UGA_Move::UGA_Move()
{
	// Movement should never be cancelled on input release
	bCancelAbilityOnInputReleased = false;
}

void UGA_Move::OnTriggeredInputAction_Implementation(const FInputActionValue &Value)
{
	// Get the 2D movement vector (forward/back, left/right)
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// Get the character
	ACattleCharacter *Character = Cast<ACattleCharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->Controller)
	{
		return;
	}

	// Apply movement input
	// Forward/backward movement
	Character->AddMovementInput(Character->GetActorForwardVector(), MovementVector.Y);

	// Left/right movement
	Character->AddMovementInput(Character->GetActorRightVector(), MovementVector.X);
}
