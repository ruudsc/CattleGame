#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Jump.generated.h"

/**
 * Gameplay Ability for player jumping.
 * Handles jump Started (jump) and Completed (stop jumping) events.
 */
UCLASS()
class CATTLEGAME_API UGA_Jump : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Jump();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
};
