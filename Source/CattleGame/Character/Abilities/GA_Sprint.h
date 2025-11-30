#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Sprint.generated.h"

/**
 * Gameplay Ability for sprinting.
 * Increases character movement speed while held, restores on release.
 */
UCLASS()
class CATTLEGAME_API UGA_Sprint : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Sprint();

    /** Sprint speed multiplier applied to MaxWalkSpeed */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sprint")
    float SprintSpeedMultiplier = 1.5f;

protected:
    /** Cached original walk speed to restore on end */
    float OriginalMaxWalkSpeed = 0.0f;

    //~ Begin UGameplayAbility Interface
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
    //~ End UGameplayAbility Interface
};
