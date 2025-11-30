// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Sprint.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "CattleGame/CattleGame.h"

UGA_Sprint::UGA_Sprint()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UCharacterMovementComponent *MovementComponent = Character->GetCharacterMovement();
    if (!MovementComponent)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Cache original speed and apply sprint multiplier
    OriginalMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
    MovementComponent->MaxWalkSpeed = OriginalMaxWalkSpeed * SprintSpeedMultiplier;

    UE_LOG(LogGASDebug, Log, TEXT("GA_Sprint: Activated - Speed increased from %.0f to %.0f"),
           OriginalMaxWalkSpeed, MovementComponent->MaxWalkSpeed);

    CommitAbility(Handle, ActorInfo, ActivationInfo);
}

void UGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // Restore original walk speed
    if (ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (UCharacterMovementComponent *MovementComponent = Character->GetCharacterMovement())
        {
            MovementComponent->MaxWalkSpeed = OriginalMaxWalkSpeed;

            UE_LOG(LogGASDebug, Log, TEXT("GA_Sprint: Ended - Speed restored to %.0f"), OriginalMaxWalkSpeed);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Sprint::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputReleased(Handle, ActorInfo, ActivationInfo);

    // End sprint when input is released
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
