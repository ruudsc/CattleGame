// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Jump.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "GameFramework/Character.h"

UGA_Jump::UGA_Jump()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Get the character and make them jump
	if (ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->Jump();
	}
}

void UGA_Jump::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// Stop jumping when input is released
	if (ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->StopJumping();
	}

	// End the ability
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
