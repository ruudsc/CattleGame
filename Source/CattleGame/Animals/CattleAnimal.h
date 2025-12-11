// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CattleGame/Volumes/ICattleFlowSource.h"
#include "CattleAnimal.generated.h"

class UCattleAbilitySystemComponent;
class UCattleAttributeSet;
class UCattleVolumeLogicComponent; // DEPRECATED - use ICattleFlowSource actors
class UGameplayAbility;			   // Forward decl
class ULassoableComponent;
class UCattleFlowSubsystem;

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
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

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
	virtual class UAbilitySystemComponent *GetAbilitySystemComponent() const override;

	// ===== FLOW SOURCE SYSTEM (NEW) =====

	/**
	 * Registers a flow source actor that is currently affecting this animal.
	 * Used by both volume-based (overlap) and spline-based (proximity) sources.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Flow")
	void AddActiveFlowSource(TScriptInterface<ICattleFlowSource> FlowSource);

	/**
	 * Unregisters a flow source actor.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Flow")
	void RemoveActiveFlowSource(TScriptInterface<ICattleFlowSource> FlowSource);

	/** Get all active flow sources affecting this animal */
	UFUNCTION(BlueprintCallable, Category = "AI|Flow")
	TArray<TScriptInterface<ICattleFlowSource>> GetActiveFlowSources() const;

	// ===== VOLUME LOGIC (DEPRECATED) =====

	/**
	 * DEPRECATED: Use AddActiveFlowSource instead.
	 * Registers a volume logic component that is currently affecting this animal.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Volumes", meta = (DeprecatedFunction, DeprecationMessage = "Use AddActiveFlowSource with ICattleFlowSource actors instead"))
	void AddActiveVolumeLogic(UCattleVolumeLogicComponent *LogicComp);

	/**
	 * DEPRECATED: Use RemoveActiveFlowSource instead.
	 * Unregisters a volume logic component.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Volumes", meta = (DeprecatedFunction, DeprecationMessage = "Use RemoveActiveFlowSource with ICattleFlowSource actors instead"))
	void RemoveActiveVolumeLogic(UCattleVolumeLogicComponent *LogicComp);

	/** DEPRECATED: Expose active volumes for AI Logic */
	TArray<UCattleVolumeLogicComponent *> GetActiveVolumeLogics() const;

protected:
	/**
	 * Active flow sources (new actor-based system).
	 * Both volume actors (overlap) and spline actors (proximity) register here.
	 */
	UPROPERTY(Transient)
	TArray<TScriptInterface<ICattleFlowSource>> ActiveFlowSources;

	/**
	 * DEPRECATED: List of Volume Logics currently overlapping with this cattle.
	 * Kept for backwards compatibility during transition.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCattleVolumeLogicComponent>> ActiveVolumeLogics;

	/**
	 * Calculates the aggregate flow direction from all active flow sources.
	 * Weights each source by its falloff and blends the results.
	 */
	FVector CalculateFlowDirection() const;

	/**
	 * DEPRECATED: Calculates the aggregate flow direction from old volume logic components.
	 */
	FVector CalculateVolumeFlowDirection() const;
};
