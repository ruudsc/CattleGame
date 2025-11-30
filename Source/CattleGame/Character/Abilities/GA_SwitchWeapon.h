#pragma once

#include "CoreMinimal.h"
#include "GA_TriggeredInput.h"
#include "GA_SwitchWeapon.generated.h"

/**
 * Gameplay Ability for cycling through weapons (Scroll Wheel).
 * Handles continuous input for weapon switching based on axis value.
 * Positive value = next weapon, Negative value = previous weapon.
 */
UCLASS()
class CATTLEGAME_API UGA_SwitchWeapon : public UGA_TriggeredInput
{
	GENERATED_BODY()

public:
	UGA_SwitchWeapon();

protected:
	virtual void OnTriggeredInputAction_Implementation(const FInputActionValue& Value) override;

private:
	/** Track last switch time to prevent rapid switching */
	float LastSwitchTime;

	/** Minimum time between switches (prevents scroll wheel spam) */
	UPROPERTY(EditAnywhere, Category = "Weapon Switch")
	float SwitchCooldown;
};
