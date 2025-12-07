#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "InventoryComponent.generated.h"

class AWeaponBase;
class ACattleCharacter;
class UGameplayAbility;
class UInputAction;

// Delegate declarations for inventory events
DECLARE_MULTICAST_DELEGATE(FOnWeaponEquipped);
DECLARE_MULTICAST_DELEGATE(FOnWeaponUnequipped);
DECLARE_MULTICAST_DELEGATE(FOnWeaponAdded);
DECLARE_MULTICAST_DELEGATE(FOnWeaponRemoved);

/** Enum for fixed weapon slot assignments */
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Revolver = 0 UMETA(DisplayName = "Revolver"),
	Lasso = 1 UMETA(DisplayName = "Lasso"),
	Dynamite = 2 UMETA(DisplayName = "Dynamite"),
	Trumpet = 3 UMETA(DisplayName = "Trumpet"),
	Pickup4 = 4 UMETA(DisplayName = "Pickup 4"),
	Pickup5 = 5 UMETA(DisplayName = "Pickup 5"),
	MaxSlots = 6
};

/**
 * Manages weapon inventory with fixed 6-slot system.
 * Slot assignments:
 *  0 = Revolver (fixed)
 *  1 = Lasso (fixed)
 *  2 = Dynamite (fixed)
 *  3 = Trumpet (fixed)
 *  4-5 = Pickup items
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CATTLEGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// ===== WEAPON SLOT MANAGEMENT =====

	/** Default inventory (design-time only).
	 *  Class per fixed slot. Index mapping: 0=Revolver, 1=Lasso, 2=Dynamite, 3=Trumpet, 4-5=Pickup slots.
	 *  Use EditDefaultsOnly so designers set this on the Blueprint class (not per-instance).
	 *  Ensure the array has at least (int32)EWeaponSlot::MaxSlots entries in your Blueprint class defaults.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory|Defaults")
	TArray<TSubclassOf<AWeaponBase>> DefaultInventory;

	/** Get the default weapon class for a given slot from the DefaultInventory array. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Defaults")
	TSubclassOf<AWeaponBase> GetDefaultWeaponClassForSlot(int32 SlotIndex) const
	{
		return DefaultInventory.IsValidIndex(SlotIndex) ? DefaultInventory[SlotIndex] : nullptr;
	}

	/**
	 * Equip a weapon from a specific slot.
	 * Returns false if slot is empty or weapon is already equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipWeapon(int32 SlotIndex);

	/**
	 * Unequip the currently equipped weapon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnequipWeapon();

	/**
	 * Add a weapon to a specific slot (owner must authorize).
	 * Returns true if successful, false if slot is occupied or invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddWeaponToSlot(AWeaponBase *Weapon, int32 SlotIndex);

	/**
	 * Add a weapon to the first available pickup slot (slots 3-5).
	 * Returns slot index if successful, -1 if no space.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddWeaponToFirstAvailableSlot(AWeaponBase *Weapon);

	/**
	 * Remove a weapon from a specific slot.
	 * Returns the weapon if found, nullptr otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AWeaponBase *RemoveWeaponFromSlot(int32 SlotIndex);

	/**
	 * Remove the currently equipped weapon and drop it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropEquippedWeapon();

	// ===== SLOT QUERIES =====

	/**
	 * Get weapon in a specific slot. Returns nullptr if empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AWeaponBase *GetWeaponInSlot(int32 SlotIndex) const;

	/**
	 * Get the currently equipped weapon. Returns nullptr if no weapon equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AWeaponBase *GetEquippedWeapon() const { return EquippedWeapon; }

	/**
	 * Get index of currently equipped slot. Returns -1 if no weapon equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetEquippedSlotIndex() const { return CurrentEquippedSlot; }

	/**
	 * Check if a slot is empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsSlotEmpty(int32 SlotIndex) const;

	/**
	 * Check if a slot is valid (0-5).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsSlotValid(int32 SlotIndex) const { return SlotIndex >= 0 && SlotIndex < (int32)EWeaponSlot::MaxSlots; }

	/**
	 * Get total weapon count across all slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetWeaponCount() const;

	// ===== WEAPON SWITCHING =====

	/**
	 * Switch to the next non-empty slot (cycles through available weapons).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CycleToNextWeapon();

	/**
	 * Switch to the previous non-empty slot (cycles through available weapons).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CycleToPreviousWeapon();

	// ===== DIRECT SLOT ACCESS =====

	/**
	 * Get all weapons in inventory (includes empty slots as nullptr).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void GetAllWeapons(TArray<AWeaponBase *> &OutWeapons) const;

	/**
	 * Get the weapon slots array (for UI display).
	 */
	const TArray<AWeaponBase *> &GetWeaponSlots() const { return WeaponSlots; }

	// ===== EVENTS =====

	/** Called when a weapon is equipped */
	FOnWeaponEquipped OnWeaponEquipped;

	/** Called when a weapon is unequipped */
	FOnWeaponUnequipped OnWeaponUnequipped;

	/** Called when weapon is added to inventory */
	FOnWeaponAdded OnWeaponAdded;

	/** Called when weapon is removed from inventory */
	FOnWeaponRemoved OnWeaponRemoved;

protected:
	virtual void BeginPlay() override;

	// ===== STATE =====

	/** Array of weapons in inventory (6 slots, nullptr = empty) */
	UPROPERTY(ReplicatedUsing = OnRep_WeaponSlots, VisibleAnywhere, Category = "Inventory|State")
	TArray<AWeaponBase *> WeaponSlots;

	/** Currently equipped weapon pointer */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon, VisibleAnywhere, Category = "Inventory|State")
	AWeaponBase *EquippedWeapon = nullptr;

	/** Index of currently equipped slot (-1 = no weapon) */
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Inventory|State")
	int32 CurrentEquippedSlot = -1;

	/** Owner character reference */
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Owner")
	ACattleCharacter *OwnerCharacter = nullptr;

	// ===== NETWORK =====

	/**
	 * Called on clients when WeaponSlots array is replicated.
	 * Ensures weapon blueprint events fire on the client.
	 */
	UFUNCTION()
	void OnRep_WeaponSlots();

	/**
	 * Called on clients when EquippedWeapon is replicated.
	 * Calls EquipWeapon on the weapon so blueprint events fire on client.
	 */
	UFUNCTION()
	void OnRep_EquippedWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
	/**
	 * Initialize default weapons from DefaultInventory array on BeginPlay (server only).
	 */
	void InitializeDefaultWeapons();

	/**
	 * Internal function to spawn a weapon and attach it to character.
	 */
	AWeaponBase *SpawnWeapon(TSubclassOf<AWeaponBase> WeaponClass);

	/**
	 * Server-side RPC to equip weapon from slot.
	 */
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(int32 SlotIndex);

	/**
	 * Server-side RPC to drop equipped weapon.
	 */
	UFUNCTION(Server, Reliable)
	void ServerDropWeapon();

	/**
	 * Find next non-empty slot going forward.
	 */
	int32 FindNextWeaponSlot() const;

	/**
	 * Find previous non-empty slot going backward.
	 */
	int32 FindPreviousWeaponSlot() const;

	// ===== WEAPON ABILITY MANAGEMENT =====

	/**
	 * Trigger the GA_WeaponEquip ability for equipping or unequipping a weapon.
	 * Handles cosmetic attachment, mesh visibility, and effects.
	 */
	void TriggerWeaponEquipAbility(AWeaponBase *Weapon, bool bIsEquipping);

	/**
	 * Grant abilities from a weapon to the owner character's ASC.
	 * Called when a weapon is equipped.
	 */
	void GrantWeaponAbilities(AWeaponBase *Weapon);

	/**
	 * Revoke abilities granted by a weapon from the owner character's ASC.
	 * Called when a weapon is unequipped.
	 */
	void RevokeWeaponAbilities(AWeaponBase *Weapon);

	/**
	 * Bind an input action to a weapon ability via InputID.
	 */
	void BindWeaponAbilityInput(class UInputAction *InputAction, int32 InputID);

	/**
	 * Unbind all weapon ability input bindings.
	 */
	void UnbindWeaponAbilityInputs();

	/** Handles for weapon input bindings (for cleanup) */
	TArray<uint32> WeaponInputBindingHandles;
};
