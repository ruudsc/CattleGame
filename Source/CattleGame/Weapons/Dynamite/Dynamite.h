#pragma once

#include "CoreMinimal.h"
#include "CattleGame/Weapons/ProjectileWeaponBase.h"
#include "Dynamite.generated.h"

class ADynamiteProjectile;
class UStaticMeshComponent;

/**
 * Dynamite Weapon - Throwable explosive.
 *
 * Mechanics:
 * - Limited ammo (3 sticks, pickable from level)
 * - Throw with arcing projectile
 * - Explodes after 5 seconds
 * - Can be eaten by enemies (state handled by projectile)
 */
UCLASS()
class CATTLEGAME_API ADynamite : public AProjectileWeaponBase
{
	GENERATED_BODY()

public:
	ADynamite();

	// ===== WEAPON INTERFACE =====

	/** Check if dynamite can be thrown (has ammo, not already thrown) */
	virtual bool CanFire() const;

	/** Check if dynamite can be reloaded */
	virtual bool CanReload() const;

	// ===== DYNAMITE STATE =====

	/** Get current ammo count */
	UFUNCTION(BlueprintCallable, Category = "Dynamite")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	/** Get max ammo capacity */
	UFUNCTION(BlueprintCallable, Category = "Dynamite")
	int32 GetMaxAmmo() const { return MaxAmmo; }

	/** Add ammo to the weapon */
	UFUNCTION(BlueprintCallable, Category = "Dynamite")
	void AddAmmo(int32 Amount);

	// ===== MESH =====

	/** Static mesh component for the dynamite visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dynamite|Visual")
	UStaticMeshComponent *DynamiteMeshComponent = nullptr;

protected:
	// ===== PROPERTIES =====

	/** Current ammo in magazine */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Dynamite|Stats")
	int32 CurrentAmmo = 3;

	/** Max ammo capacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamite|Stats")
	int32 MaxAmmo = 10;

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamite|Projectile")
	TSubclassOf<ADynamiteProjectile> ProjectileClass;

	/** Force applied when throwing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamite|Projectile")
	float ThrowForce = 1500.0f;

	/** Explosion radius after detonation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamite|Explosion")
	float ExplosionRadius = 500.0f;

	/** Damage dealt by explosion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamite|Explosion")
	float ExplosionDamage = 100.0f;

	/** Time until explosion after projectile is spawned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamite|Explosion")
	float FuseTime = 5.0f;

	// ===== NETWORK =====

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	// Authoritative server processing for projectile spawn/launch
	virtual void OnServerFire(const FVector &SpawnLocation, const FVector &LaunchDirection) override;
};
