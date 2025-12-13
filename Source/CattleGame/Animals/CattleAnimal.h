// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Areas/CattleAreaSubsystem.h"
#include "CattleAnimal.generated.h"

class ULassoableComponent;
class UCattleAnimalMovementComponent;
class UCattleAbilitySystemComponent;
class UAnimalAttributeSet;

/**
 * ACattleAnimal
 *
 * Base class for all cattle/animal characters in the game.
 * Features:
 * - Custom movement component with physics influence support
 * - Gameplay Ability System integration for behavior states
 * - Area-based AI influence (graze, panic, avoid, flow)
 * - Lasso target support
 */
UCLASS(Blueprintable, BlueprintType)
class CATTLEGAME_API ACattleAnimal : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACattleAnimal(const FObjectInitializer &ObjectInitializer);

	// ===== IAbilitySystemInterface =====

	virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

	// ===== Getters =====

	/** Get the custom animal movement component */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal")
	UCattleAnimalMovementComponent *GetAnimalMovement() const { return AnimalMovement; }

	/** Get the animal attribute set */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal")
	UAnimalAttributeSet *GetAnimalAttributes() const { return AnimalAttributes; }

	// ===== Area Influence =====

	/** Update area influences based on current location */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal|Area")
	void UpdateAreaInfluences();

	/** Get the current primary area influence */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal|Area")
	FCattleAreaInfluence GetCurrentAreaInfluence() const;

	/** Apply a physics impulse to the animal (e.g., from lasso, explosion) */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal")
	void ApplyPhysicsImpulse(FVector Impulse, bool bVelocityChange = false);

	// ===== Fear/Panic System =====

	/** Add fear to the animal (triggers panic at threshold) */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal|Fear")
	void AddFear(float Amount);

	/** Add calm to the animal (reduces fear) */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal|Fear")
	void AddCalm(float Amount);

	/** Check if animal is currently panicked */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal|Fear")
	bool IsPanicked() const;

	/** Get current fear level as percentage (0-1) */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal|Fear")
	float GetFearPercent() const;

	// ===== Lasso Reactions =====

	/** Check if animal is currently lassoed */
	UFUNCTION(BlueprintCallable, Category = "Cattle Animal|Lasso")
	bool IsLassoed() const { return bIsLassoed; }

	/** Called when this animal is captured by a lasso */
	UFUNCTION()
	void OnLassoCaptured(AActor *LassoOwner);

	/** Called when this animal is released from a lasso */
	UFUNCTION()
	void OnLassoReleased();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
	virtual void PossessedBy(AController *NewController) override;

	// ===== GAS Initialization =====

	/** Initialize the ability system component */
	virtual void InitializeAbilitySystem();

	/** Grant default abilities */
	virtual void GrantDefaultAbilities();

	/** Apply default gameplay effects */
	virtual void ApplyDefaultEffects();

	// ===== Area Processing =====

	/** Process area influences and update movement/behavior */
	void ProcessAreaInfluences(float DeltaTime);

	/** Decay fear naturally over time */
	void DecayFear(float DeltaTime);

	// ===== Components =====

	/** Lassoable component for lasso target support */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Lasso")
	TObjectPtr<ULassoableComponent> LassoableComponent;

	/** Custom movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Movement")
	TObjectPtr<UCattleAnimalMovementComponent> AnimalMovement;

	/** Ability system component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Abilities")
	TObjectPtr<UCattleAbilitySystemComponent> AbilitySystemComponent;

	/** Animal-specific attributes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Abilities")
	TObjectPtr<UAnimalAttributeSet> AnimalAttributes;

	// ===== Configuration =====

	/** Default abilities to grant on spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cattle Animal|Abilities")
	TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;

	/** Default effects to apply on spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cattle Animal|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> DefaultEffects;

	/** How often to update area influences (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cattle Animal|Area")
	float AreaUpdateInterval = 0.1f;

	/** Socket name for lasso attachment */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cattle Animal|Lasso")
	FName LassoAttachSocket = FName("pelvis");

	/** Fear to add when lassoed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cattle Animal|Lasso")
	float LassoFearAmount = 50.0f;

private:
	/** Timer for area influence updates */
	float AreaUpdateTimer = 0.0f;

	/** Is currently lassoed */
	bool bIsLassoed = false;

	/** Cached area subsystem reference */
	UPROPERTY(Transient)
	TObjectPtr<UCattleAreaSubsystem> CachedAreaSubsystem;

	/** Current area influence */
	FCattleAreaInfluence CurrentInfluence;
};
