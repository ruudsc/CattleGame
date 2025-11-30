#pragma once

#include "CoreMinimal.h"
#include "GA_TriggeredInput.h"
#include "GA_Move.generated.h"

/**
 * Gameplay Ability for player movement (WASD / Left Stick).
 * Handles continuous movement input and applies it to the character.
 */
UCLASS()
class CATTLEGAME_API UGA_Move : public UGA_TriggeredInput
{
	GENERATED_BODY()

public:
	UGA_Move();

protected:
	virtual void OnTriggeredInputAction_Implementation(const FInputActionValue& Value) override;
};
