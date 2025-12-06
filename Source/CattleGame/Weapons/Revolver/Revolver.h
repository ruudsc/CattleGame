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

	/** Is the weapon currently being reloaded? */
	UPROPERTY(ReplicatedUsing = OnRep_IsReloading, BlueprintReadOnly, Category = "Revolver|State")
	bool bIsReloading = false;

	/** Timestamp of last fire for fire rate limiting */
	float LastFireTime = -9999.0f;

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
	// ===== MESH =====

	/** Static mesh component for the revolver visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolver|Visual")
	UStaticMeshComponent *RevolverMeshComponent = nullptr;

protected:
	virtual void BeginPlay() override;

	// Authoritative server processing for hitscan fire (trace and damage)
	virtual void OnServerFire(const FVector &TraceStart, const FVector &TraceDir) override;

private:
	/**
	 * Apply damage to a hit actor.
	 */
	void ApplyDamageToActor(AActor *HitActor, const FVector &HitLocation, const FVector &ShotDirection);

	/** Handle reload timer completion */
	FTimerHandle ReloadTimerHandle;
};
