#pragma once

#include "CoreMinimal.h"
#include "CattleGame/Weapons/HitscanWeaponBase.h"
#include "Revolver.generated.h"

class UParticleSystem;
class USoundBase;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * Revolver weapon - hitscan weapon with 6-round magazine.
 * Fires instantly with line traces, deals damage to hit targets.
 */
UCLASS()
class CATTLEGAME_API ARevolver : public AHitscanWeaponBase
{
	GENERATED_BODY()

public:
	ARevolver();

	// ===== SOCKET CONSTANTS =====

	/** Socket name for revolver attachment on character skeleton */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolver|Attachment")
	FName RevolverHandSocketName = FName(TEXT("HandGrip_R"));

	// ===== WEAPON STATS =====

	/** Maximum ammo this weapon can hold */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Revolver|Stats")
	int32 MaxAmmo = 6;

	/** Current ammo count */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, BlueprintReadOnly, Category = "Revolver|Stats")
	int32 CurrentAmmo = 6;

	/** Damage dealt per shot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Stats")
	float DamageAmount = 25.0f;

	/** Fire rate in shots per second (2.0 = 2 shots per second) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Stats")
	float FireRate = 2.0f;

	/** Time to reload in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Stats")
	float ReloadTime = 2.0f;

	/** Range for hitscan in units */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Stats")
	float WeaponRange = 5000.0f;

	/** Spread radius for bullet traces in degrees */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Stats")
	float WeaponSpread = 0.0f; // Perfect accuracy for revolver

	// ===== STATE TRACKING =====

	/** Is the weapon currently equipped and in use? */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Revolver|State")
	bool bIsEquipped = false;

	/** Is the weapon currently being reloaded? */
	UPROPERTY(ReplicatedUsing = OnRep_IsReloading, BlueprintReadOnly, Category = "Revolver|State")
	bool bIsReloading = false;

	/** Timestamp of last fire for fire rate limiting */
	float LastFireTime = -9999.0f;

	/** Muzzle flash particle effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|VFX")
	UParticleSystem *MuzzleFlashEffect = nullptr;

	/** Bullet impact effect at trace end point */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|VFX")
	UParticleSystem *ImpactEffect = nullptr;

	/** Fire sound effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Audio")
	USoundBase *FireSound = nullptr;

	/** Reload sound effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Audio")
	USoundBase *ReloadSound = nullptr;

	/** Empty magazine click sound */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Audio")
	USoundBase *EmptySound = nullptr;

	/** Whether to apply hit reactions to hit actors */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Revolver|Damage")
	bool bApplyHitReaction = true;

	// ===== UTILITY FUNCTIONS =====

	/**
	 * Check if the weapon can fire (has ammo, not reloading, fire rate check).
	 */
	UFUNCTION(BlueprintCallable, Category = "Revolver")
	bool CanFire() const;

	/**
	 * Check if the weapon needs ammo and can be reloaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Revolver")
	bool CanReload() const;

	/** Get current ammo as string for UI */
	UFUNCTION(BlueprintCallable, Category = "Revolver")
	FString GetAmmoString() const;

	/**
	 * Called at the end of reload sequence to refill ammo.
	 */
	UFUNCTION(BlueprintCallable, Category = "Revolver")
	void OnReloadComplete();

	// ===== NETWORK =====

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	/** Notifies clients when ammo changes */
	UFUNCTION()
	void OnRep_CurrentAmmo();

	/** Notifies clients when reload state changes */
	UFUNCTION()
	void OnRep_IsReloading();

public:
	/**
	 * Override EquipWeapon to attach to character hand
	 */
	virtual void EquipWeapon() override;

	/**
	 * Override UnequipWeapon to detach from character
	 */
	virtual void UnequipWeapon() override;

	/**
	 * Override Fire to ensure C++ implementation runs even if blueprint event is empty
	 */
	virtual void Fire() override;

	// ===== MESH =====

	/** Static mesh component for the revolver visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolver|Visual")
	UStaticMeshComponent *RevolverMeshComponent = nullptr;

protected:
	virtual void BeginPlay() override;

	// Authoritative server processing for hitscan fire (trace and damage)
	virtual void OnServerFire(const FVector &TraceStart, const FVector &TraceDir) override;

	// Listen to weapon base events and implement game logic
	UFUNCTION()
	void OnWeaponEquipped_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

	UFUNCTION()
	void OnWeaponUnequipped_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

	UFUNCTION()
	void OnWeaponFired_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

	UFUNCTION()
	void OnReloadStarted_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh);

private:
	/**
	 * Perform hitscan trace and apply damage to hit targets.
	 * Called from OnFire via animation notify.
	 */
	UFUNCTION()
	void PerformLineTrace();

	/**
	 * Apply damage to a hit actor.
	 */
	void ApplyDamageToActor(AActor *HitActor, const FVector &HitLocation, const FVector &ShotDirection);

	/**
	 * Play muzzle flash effect at weapon socket.
	 */
	void PlayMuzzleFlash();

	/**
	 * Play impact effect at trace hit location.
	 */
	void PlayImpactEffect(const FVector &ImpactLocation, const FVector &ImpactNormal);

	/**
	 * Play fire sound.
	 */
	void PlayFireSound();

	/**
	 * Get the trace start and direction from camera.
	 */
	void GetTraceStartAndDirection(FVector &OutStart, FVector &OutDirection) const;

	/** Attach revolver mesh to character hand socket */
	void AttachToCharacterHand();

	/** Handle reload timer completion */
	FTimerHandle ReloadTimerHandle;
};
