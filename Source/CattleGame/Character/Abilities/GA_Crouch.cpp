// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Crouch.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "GameFramework/Character.h"

UGA_Crouch::UGA_Crouch()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Crouch::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Get the character and make them crouch
	if (ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->Crouch();
	}
}

void UGA_Crouch::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// Stop crouching when input is released
	if (ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->UnCrouch();
	}

	// End the ability
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
