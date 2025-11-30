#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Delegates/Delegate.h"
#include "DynamiteProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystem;

/**
 * Dynamite Projectile - Throwable explosive that detonates after a delay.
 *
 * States:
 * - Flying: In transit to target
 * - Fusing: Started timer, waiting to explode
 * - Eaten: Enemy ate it (state for future AI use)
 */
UENUM(BlueprintType)
enum class EDynamiteState : uint8
{
	Flying UMETA(DisplayName = "Flying"),
	Fusing UMETA(DisplayName = "Fusing"),
	Eaten UMETA(DisplayName = "Eaten")
};

UCLASS()
class CATTLEGAME_API ADynamiteProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADynamiteProjectile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== PROJECTILE CONTROL =====

	/** Launch the projectile with a direction and force */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Launch(const FVector &Direction, float Force);

	/** Set explosion properties before launch */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void SetExplosionProperties(float Radius, float Damage);

	/** Set the fuse time */
	UFUNCTION(BlueprintCallable, Category = "Explosion")
	void SetFuseTime(float NewFuseTime);

	// ===== STATE QUERIES =====

	/** Get current projectile state */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	EDynamiteState GetProjectileState() const { return CurrentState; }

	/** Check if projectile has been eaten */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	bool HasBeenEaten() const { return CurrentState == EDynamiteState::Eaten; }

	// ===== EVENTS =====

	/** Called when dynamite explodes */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDynamiteExplodedDelegate);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FDynamiteExplodedDelegate OnExploded;

	/** Called when dynamite is eaten by an enemy */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDynamiteEatenDelegate);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FDynamiteEatenDelegate OnEaten;

protected:
	// ===== COMPONENTS =====

	/** Root collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent *CollisionSphere;

	/** Mesh component for visual representation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent *MeshComponent;

	/** Movement component for physics */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent *ProjectileMovement;

	// ===== EXPLOSION PROPERTIES =====

	/** Radius of explosion damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionRadius = 500.0f;

	/** Damage dealt by explosion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionDamage = 100.0f;

	/** Time until explosion after projectile is spawned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float FuseTime = 5.0f;

	/** Particle system for explosion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|VFX")
	UParticleSystem *ExplosionParticles;

	/** Particle system for fuse burning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion|VFX")
	UParticleSystem *FuseParticles;

	// ===== STATE =====

	/** Current projectile state */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	EDynamiteState CurrentState = EDynamiteState::Flying;

	/** Timer handle for explosion */
	FTimerHandle ExplosionTimerHandle;

private:
	/** Handle collision with world (ground, walls) */
	UFUNCTION()
	void OnCollision(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
					 FVector NormalImpulse, const FHitResult &Hit);

	/** Explode and apply damage */
	void Explode();

	/** Get lifetime replicated properties */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
