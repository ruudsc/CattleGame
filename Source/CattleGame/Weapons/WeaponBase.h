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

	/** Whether this weapon is currently equipped (controls visibility via OnRep) */
	UPROPERTY(ReplicatedUsing = OnRep_IsEquipped, BlueprintReadOnly, Category = "Weapon|State")
	bool bIsEquipped = false;

	/** Called when bIsEquipped replicates - updates visibility */
	UFUNCTION()
	void OnRep_IsEquipped();

	/** Owner character reference (replicated for clients that need owner context). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Owner")
	ACattleCharacter *OwnerCharacter = nullptr;

	/** Offset transform when attached to the first-person mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Attachment")
	FTransform FirstPersonAttachmentOffset = FTransform::Identity;

	/** Offset transform when attached to the third-person mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Attachment")
	FTransform ThirdPersonAttachmentOffset = FTransform::Identity;

	/** Socket name on character skeleton for weapon attachment */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Attachment")
	FName AttachmentSocketName = FName(TEXT("HandGrip_R"));

	/**
	 * Get the socket name for attaching this weapon to character.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	FName GetAttachmentSocketName() const { return AttachmentSocketName; }

	/**
	 * Attach weapon to character's hand socket on active mesh.
	 * Uses AttachmentSocketName property.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	void AttachToCharacterHand();

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

	// ===== HELPERS =====

	/**
	 * Set the owner character for this weapon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetOwnerCharacter(ACattleCharacter *Character);

	/**
	 * Get the owner character for this weapon (cached or resolved from Owner).
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ACattleCharacter *GetOwnerCharacter() const;

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
	/** Optional skeletal mesh component for this weapon. Set by derived classes or blueprints. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Mesh")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh = nullptr;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
};
