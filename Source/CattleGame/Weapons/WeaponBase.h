#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputActionValue.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "WeaponBase.generated.h"

class ACattleCharacter;
class USkeletalMeshComponent;
class UInputAction;

/**
 * Struct that pairs a GameplayAbility with an Input Action for weapon abilities.
 * Used by weapons to define which abilities they grant and their input bindings.
 */
USTRUCT(BlueprintType)
struct FWeaponAbilityInfo
{
	GENERATED_USTRUCT_BODY()

	/** The Gameplay Ability class to grant when weapon is equipped */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** The Enhanced Input Action that triggers this ability */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UInputAction> InputAction;

	/** Check if this info is valid */
	bool IsValid() const
	{
		return AbilityClass != nullptr;
	}

	FWeaponAbilityInfo()
		: AbilityClass(nullptr), InputAction(nullptr)
	{
	}
};

/**
 * Base class for all weapons in the game.
 * Handles weapon attachment, firing, ammo management, and reloading.
 */
UCLASS(Abstract, Blueprintable)
class CATTLEGAME_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	// ===== WEAPON IDENTIFICATION =====

	/** Unique ID for this weapon slot (0 = Revolver, 1 = Lasso, 2 = Dynamite, 3-5 = pickups) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Config")
	int32 WeaponSlotID = -1;

	/** Display name for this weapon (e.g., "Revolver", "Lasso") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Config")
	FString WeaponName = "Unnamed Weapon";

	/** Owner character reference (replicated for clients that need owner context). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Owner")
	ACattleCharacter *OwnerCharacter = nullptr;

	/** Offset transform when attached to the first-person mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Attachment")
	FTransform FirstPersonAttachmentOffset = FTransform::Identity;

	/** Offset transform when attached to the third-person mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Attachment")
	FTransform ThirdPersonAttachmentOffset = FTransform::Identity;

	// ===== ABILITY SYSTEM =====

	/**
	 * Abilities granted to the character when this weapon is equipped.
	 * Each ability can optionally be bound to an input action.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Abilities")
	TArray<FWeaponAbilityInfo> WeaponAbilities;

	/**
	 * Get the ability info array for this weapon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Abilities")
	const TArray<FWeaponAbilityInfo> &GetWeaponAbilities() const { return WeaponAbilities; }

	/**
	 * Handles for abilities granted by this weapon (used for cleanup on unequip).
	 * Managed by InventoryComponent - do not modify directly.
	 */
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	// ===== CORE FUNCTIONALITY =====
	// These are simple event triggers - all logic is handled in child classes or blueprints

	/**
	 * Called when weapon is equipped. Triggers OnWeaponEquipped event for blueprints.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void EquipWeapon();

	/**
	 * Called when weapon is unequipped. Triggers OnWeaponUnequipped event for blueprints.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void UnequipWeapon();

	/**
	 * Called when weapon fires. Triggers OnWeaponFired event for blueprints.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Fire();

	/**
	 * Called when weapon starts reloading. Triggers OnReloadStarted event for blueprints.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Reload();

	/**
	 * Set the owner character for this weapon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetOwnerCharacter(ACattleCharacter *Character);

	/**
	 * Helper for blueprints: choose the attachment offset that matches the supplied mesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	FTransform GetAttachmentOffsetForMesh(const USkeletalMeshComponent *Mesh) const;

	/**
	 * Get the skeletal mesh component of this weapon (for socket queries, VFX, etc.).
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent *GetWeaponMesh() const;

	// ===== NETWORK =====

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	virtual bool IsNetRelevantFor(const AActor *RealViewer, const AActor *Viewer, const FVector &SrcLocation) const override;

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	// ===== BLUEPRINT EVENTS =====
	// These events are called from C++ and can be listened to in Blueprint Event Graph

	/**
	 * Called when the weapon is equipped. Blueprints should handle mesh visibility and animations.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Events", meta = (DisplayName = "On Weapon Equipped"))
	void OnWeaponEquipped(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

	/**
	 * Called when the weapon is unequipped. Blueprints should handle mesh hiding and cleanup.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Events", meta = (DisplayName = "On Weapon Unequipped"))
	void OnWeaponUnequipped(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

	/**
	 * Called when the weapon fires. Blueprints should handle animations, VFX, and SFX.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Events", meta = (DisplayName = "On Weapon Fired"))
	void OnWeaponFired(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

	/**
	 * Called when reload starts. Blueprints should handle reload animations and VFX.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Events", meta = (DisplayName = "On Reload Started"))
	void OnReloadStarted(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

	/**
	 * Called when reload completes. Blueprints can handle reload completion VFX/SFX.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Events", meta = (DisplayName = "On Reload Completed"))
	void OnReloadCompleted(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);
};
