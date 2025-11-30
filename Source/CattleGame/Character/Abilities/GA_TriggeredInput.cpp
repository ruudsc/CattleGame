
// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_TriggeredInput.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "CattleGame/CattleGame.h"

UGA_TriggeredInput::UGA_TriggeredInput()
	: bCancelAbilityOnInputReleased(true)
{
	// These abilities are instanced per actor and local only (no server execution needed for movement)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_TriggeredInput::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bool bSuccess = false;

	// Get the character and its input component
	if (const ACattleCharacter *PlayerCharacter = Cast<ACattleCharacter>(GetAvatarActorFromActorInfo()))
	{
		// Only the locally controlled pawn should bind Enhanced Input events. On server/remote proxies, skip binding.
		if (!PlayerCharacter->IsLocallyControlled())
		{
			// Treat as success so mirrored server ability doesn't cancel and spam logs
			bSuccess = true;
			CommitAbility(Handle, ActorInfo, ActivationInfo);
			return;
		}

		EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerCharacter->InputComponent);
		if (EnhancedInputComponent)
		{
			// Find the matching input action from the CharacterAbilities array
			const TArray<FCharacterAbilityInfo> &CharacterAbilities = PlayerCharacter->GetCharacterAbilities();
			for (const FCharacterAbilityInfo &AbilityInfo : CharacterAbilities)
			{
				// Find the entry that matches this ability class
				const UClass *ThisClass = GetClass();
				const UClass *ConfiguredClass = AbilityInfo.GameplayAbilityClass;
				if (AbilityInfo.IsValid() && ThisClass && ConfiguredClass && (ThisClass->IsChildOf(ConfiguredClass) || ConfiguredClass->IsChildOf(ThisClass)))
				{
					// Dynamically bind to the Triggered event for continuous input
					const FEnhancedInputActionEventBinding &TriggeredBinding = EnhancedInputComponent->BindAction(
						AbilityInfo.InputAction,
						ETriggerEvent::Triggered,
						this,
						&UGA_TriggeredInput::OnTriggeredInputAction);
					const uint32 TriggeredHandle = TriggeredBinding.GetHandle();
					TriggeredEventHandles.AddUnique(TriggeredHandle);

					bSuccess = true;
				}
			}

			if (!bSuccess)
			{
				UE_LOG(LogGASDebug, Warning, TEXT("TriggeredInput: No matching InputAction found for ability class %s. Ensure it's listed in CharacterAbilities array."), *GetNameSafe(GetClass()));
			}
		}
		else
		{
			UE_LOG(LogGASDebug, Warning, TEXT("TriggeredInput: EnhancedInputComponent is null on %s during ActivateAbility."), *GetNameSafe(PlayerCharacter));
		}
	}
	else
	{
		UE_LOG(LogGASDebug, Warning, TEXT("TriggeredInput: AvatarActor is not a valid ACattleCharacter during ActivateAbility."));
	}

	if (bSuccess)
	{
		// Commit the ability (costs, cooldowns, etc.)
		CommitAbility(Handle, ActorInfo, ActivationInfo);
	}
	else
	{
		// Failed to set up - cancel the ability
		constexpr bool bReplicateCancelAbility = true;
		CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
	}
}

void UGA_TriggeredInput::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Clean up the dynamically bound Triggered events
	if (EnhancedInputComponent)
	{
		for (const uint32 EventHandle : TriggeredEventHandles)
		{
			EnhancedInputComponent->RemoveBindingByHandle(EventHandle);
		}

		EnhancedInputComponent = nullptr;
	}

	TriggeredEventHandles.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_TriggeredInput::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// Cancel the ability when input is released (if configured to do so)
	if (bCancelAbilityOnInputReleased)
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
	}
}

void UGA_TriggeredInput::OnTriggeredInputAction_Implementation(const FInputActionValue &Value)
{
	// Base implementation does nothing - override in child classes
}
