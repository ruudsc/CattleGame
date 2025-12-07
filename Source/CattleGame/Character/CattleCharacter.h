// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "CattleCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInventoryComponent;
class AWeaponBase;
class UCattleAbilitySystemComponent;
class UCattleAttributeSet;
struct FInputActionValue;

/**
 * Struct that pairs a GameplayAbility with an Enhanced Input Action for character abilities.
 * These are core character abilities (Move, Look, Jump, Crouch, EquipWeaponSlot) that persist
 * for the character's lifetime. InputIDs are auto-generated in range 1000-1999 to avoid
 * conflicts with weapon abilities (100+).
 */
USTRUCT(BlueprintType)
struct FCharacterAbilityInfo
{
	GENERATED_USTRUCT_BODY()

	/** The Gameplay Ability class to grant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> GameplayAbilityClass;

	/** The Enhanced Input Action that triggers this ability */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UInputAction> InputAction;

	/** Auto-generated InputID - DO NOT EDIT MANUALLY */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability")
	int32 InputID;

	/** Check if this info is valid */
	bool IsValid() const
	{
		return GameplayAbilityClass && InputAction;
	}

	FCharacterAbilityInfo()
		: InputID(INDEX_NONE)
	{
	}
};

/**
 * Enum for controlling mesh visibility modes
 * Used for development/design iteration and debugging
 */
UENUM(BlueprintType)
enum class EMeshVisibilityMode : uint8
{
	/** Normal mode: owner sees FP, others see TP (multiplayer-safe) */
	Normal UMETA(DisplayName = "Normal (Owner=FP, Others=TP)"),

	/** Show both first and third-person meshes */
	ShowBoth UMETA(DisplayName = "Show Both FP and TP"),

	/** Show only first-person mesh */
	FirstPersonOnly UMETA(DisplayName = "First Person Only"),

	/** Show only third-person mesh */
	ThirdPersonOnly UMETA(DisplayName = "Third Person Only")
};

/**
 * First-person cattle character with separate FP/TP mesh visibility for multiplayer
 * - Local player sees first-person mesh (arms/body view)
 * - Remote players see third-person mesh (full body)
 *
 * Supports development modes for game designers to easily toggle mesh visibility
 * and work on animations, sockets, and attachments in both perspectives.
 *
 * Implements the Gameplay Ability System for managing character abilities and attributes.
 */
UCLASS()
class CATTLEGAME_API ACattleCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACattleCharacter();

protected:
	// ===== ABILITY SYSTEM =====
	/** Ability system component for managing abilities and attributes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UCattleAbilitySystemComponent> AbilitySystemComponent;

	/**
	 * Core character abilities - granted once at initialization, never removed.
	 * These include: Movement, Look, Jump, Crouch, Weapon Slot Equipping.
	 * InputIDs: 1000-1999 (auto-generated to avoid conflict with weapon abilities 100+)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<FCharacterAbilityInfo> CharacterAbilities;

	// Mesh Components
	/** First-person skeletal mesh (visible only to owner) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	// Note: Third-person mesh uses the inherited GetMesh() from ACharacter base class

	// Camera Components
	/** First-person camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Mesh Offsets (can be overridden in blueprints)
	/** Offset for first-person camera from capsule root */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector FirstPersonCameraOffset = FVector(0.0f, 0.0f, 68.0f);

	/** Offset for first-person mesh from camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	FVector FirstPersonMeshOffset = FVector(0.0f, 0.0f, -68.0f);

	/** Rotation for first-person mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	FRotator FirstPersonMeshRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Development/Debug Properties
	/** Override automatic mesh visibility logic and use the CurrentVisibilityMode instead */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Mesh Visibility")
	bool bOverrideMeshVisibility = false;

	/** Current mesh visibility mode (for development and design iteration) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Mesh Visibility", meta = (EditCondition = "bOverrideMeshVisibility"))
	EMeshVisibilityMode CurrentVisibilityMode = EMeshVisibilityMode::Normal;

protected:
	// Input Properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// ===== INVENTORY COMPONENT =====
	/** Manages weapon inventory and equipment */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

protected:
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

	/**
	 * Set the mesh visibility mode for development/design iteration
	 * Allows game designers to toggle between different visibility modes in the editor
	 * @param NewMode The visibility mode to apply
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|Mesh Visibility")
	void SetMeshVisibilityMode(EMeshVisibilityMode NewMode);

	// ===== ABILITY SYSTEM ACCESSORS =====

	/**
	 * IAbilitySystemInterface implementation - required for GAS integration
	 */
	virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

	// ===== INVENTORY & WEAPON ACCESSORS =====

	/**
	 * Get the inventory component for weapon management.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryComponent *GetInventoryComponent() const { return InventoryComponent; }

	/**
	 * Get the first-person mesh component.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh")
	USkeletalMeshComponent *GetFirstPersonMesh() const { return FirstPersonMesh; }

	/**
	 * Get the currently active mesh for weapon attachments (FP vs TP).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh")
	USkeletalMeshComponent *GetActiveCharacterMesh() const;

	/**
	 * Get the first-person camera component.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	UCameraComponent *GetFirstPersonCameraComponent() const { return FirstPersonCamera; }

	/** Expose default mapping context for controller fallback */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Input")
	UInputMappingContext *GetDefaultMappingContext() const { return DefaultMappingContext; }

	/**
	 * Check if a specific gameplay tag is active on the character.
	 * Used by animation blueprints to query character state (firing, reloading, etc.).
	 * @param InTag The gameplay tag to check for
	 * @return True if the tag is active, false otherwise
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	bool HasGameplayTag(FGameplayTag InTag) const;

	/**
	 * Get the character abilities array.
	 */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	const TArray<FCharacterAbilityInfo> &GetCharacterAbilities() const { return CharacterAbilities; }

#if WITH_EDITOR
	/** Auto-generate InputIDs when character abilities are edited */
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

private:
	/** Configures mesh visibility for first-person and third-person views */
	void SetupMeshVisibility();

	// ===== VIEW ROTATION REPLICATION =====
	/** Replicated view/control rotation (yaw+pitch) for remote clients to drive third-person orientation & aim offsets. */
	UPROPERTY(ReplicatedUsing = OnRep_ViewRotation)
	FRotator ReplicatedViewRotation;

	/** Timestamp of last server update to throttle unreliable RPCs. */
	float LastViewRotationSendTime = 0.f;

	/** Called on clients when ReplicatedViewRotation changes. */
	UFUNCTION()
	void OnRep_ViewRotation();

public:
	/** Send updated control rotation from owning client to server (unreliable, high-frequency). */
	UFUNCTION(Server, Unreliable)
	void ServerSetViewRotation(const FRotator &NewRotation);
	void ServerSetViewRotation_Implementation(const FRotator &NewRotation);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	// ===== ABILITY INPUT HANDLERS (GAS Integration) =====

	/**
	 * Initialize the ability system and grant abilities from the Data Asset.
	 * Called in BeginPlay on the server.
	 */
	void InitAbilitySystem();

public:
	/**
	 * Called when an ability input is pressed.
	 * Forwards the input to the Ability System Component.
	 */
	void OnAbilityInputPressed(int32 InputID);

	/**
	 * Called when an ability input is released.
	 * Forwards the input to the Ability System Component.
	 */
	void OnAbilityInputReleased(int32 InputID);

protected:
	/** Resolve which mesh visibility mode should currently apply. */
	EMeshVisibilityMode ResolveMeshVisibilityMode() const;
};
