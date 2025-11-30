#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Crouch.generated.h"

/**
 * Gameplay Ability for player crouching.
 * Handles crouch Started (begin crouch) and Completed (end crouch) events.
 */
UCLASS()
class CATTLEGAME_API UGA_Crouch : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crouch();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
};
