// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Look.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "InputActionValue.h"

UGA_Look::UGA_Look()
{
	// Look should never be cancelled on input release
	bCancelAbilityOnInputReleased = false;
}

void UGA_Look::OnTriggeredInputAction_Implementation(const FInputActionValue &Value)
{
	// Get the 2D look vector (yaw/pitch)
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	// Get the character
	ACattleCharacter *Character = Cast<ACattleCharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->Controller)
	{
		return;
	}

	// Apply camera rotation locally
	Character->AddControllerYawInput(LookAxisVector.X);
	Character->AddControllerPitchInput(LookAxisVector.Y * -1.0f);

	// Send updated rotation to server for replication (unreliable high-frequency)
	if (!Character->HasAuthority() && Character->Controller)
	{
		const FRotator ControlRot = Character->Controller->GetControlRotation();
		Character->ServerSetViewRotation(ControlRot);
	}
}
