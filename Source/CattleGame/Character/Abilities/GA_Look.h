#pragma once

#include "CoreMinimal.h"
#include "GA_TriggeredInput.h"
#include "GA_Look.generated.h"

/**
 * Gameplay Ability for player camera look (Mouse / Right Stick).
 * Handles continuous camera rotation input.
 */
UCLASS()
class CATTLEGAME_API UGA_Look : public UGA_TriggeredInput
{
	GENERATED_BODY()

public:
	UGA_Look();

protected:
	virtual void OnTriggeredInputAction_Implementation(const FInputActionValue& Value) override;
};
