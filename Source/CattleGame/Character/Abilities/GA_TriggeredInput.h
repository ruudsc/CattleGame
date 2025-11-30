#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_TriggeredInput.generated.h"

struct FInputActionValue;
class UEnhancedInputComponent;

/**
 * Base class for abilities that need to handle continuous/triggered input events.
 *
 * This is necessary for abilities like Move and Look where the input is continuously
 * updated every frame while the button/stick is held.
 *
 * Regular GAS abilities only handle Started/Completed events. This class adds support
 * for the Triggered event by dynamically binding to the input action when the ability activates.
 */
UCLASS(Abstract)
class CATTLEGAME_API UGA_TriggeredInput : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TriggeredInput();

	/**
	 * Should the ability automatically cancel when input is released?
	 * Default: true
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bCancelAbilityOnInputReleased;

protected:
	/** Cached reference to the Enhanced Input Component */
	UPROPERTY()
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent;

	/** Handles to the dynamically bound Triggered events */
	TArray<uint32> TriggeredEventHandles;

	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~ End UGameplayAbility Interface

	/**
	 * Called every frame while the input action is triggered (held down).
	 * Override this in child classes to implement the continuous behavior.
	 *
	 * @param Value The input action value (Vector2D for movement, float for single axis, etc.)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnTriggeredInputAction(const FInputActionValue& Value);
	virtual void OnTriggeredInputAction_Implementation(const FInputActionValue& Value);
};
