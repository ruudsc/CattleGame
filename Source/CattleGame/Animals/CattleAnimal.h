// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CattleAnimal.generated.h"

class UCattleAbilitySystemComponent;
class UCattleAttributeSet;
class UCattleVolumeLogicComponent;
class UGameplayAbility; // Forward decl
class ULassoableComponent;


UCLASS()
class CATTLEGAME_API ACattleAnimal : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACattleAnimal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ===== LASSO SUPPORT =====

	/** Lassoable component for lasso target support */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lasso")
	TObjectPtr<ULassoableComponent> LassoableComponent;

	// ===== ABILITY SYSTEM =====
	
	/** Ability system component for managing abilities and attributes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UCattleAbilitySystemComponent> AbilitySystemComponent;

	/** The AttributeSet for this character */
	UPROPERTY()
	TObjectPtr<UCattleAttributeSet> AttributeSet;

	// IAbilitySystemInterface implementation
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ===== VOLUME LOGIC =====

	/**
	 * Registers a volume logic component that is currently affecting this animal.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Volumes")
	void AddActiveVolumeLogic(UCattleVolumeLogicComponent* LogicComp);

	/**
	 * Unregisters a volume logic component.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Volumes")
	void RemoveActiveVolumeLogic(UCattleVolumeLogicComponent* LogicComp);

	/** Expose active volumes for AI Logic */
	TArray<UCattleVolumeLogicComponent*> GetActiveVolumeLogics() const;

protected:
	/**
	 * List of Volume Logics currently overlapping with this cattle.
	 * Polled in Tick to determine influence (e.g. flow direction).
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCattleVolumeLogicComponent>> ActiveVolumeLogics;

	/**
	 * Calculates the aggregate flow direction from all active logic components.
	 */
	FVector CalculateVolumeFlowDirection() const;

};
