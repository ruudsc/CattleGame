#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LassoProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class ALasso;

/**
 * Lasso Projectile - Invisible arc projectile for hit detection.
 *
 * RDR2-style implementation:
 * - No visible mesh during flight (throw animation is cosmetic)
 * - Gravity-based arc trajectory
 * - Aim assist with smooth target locking
 * - On hit: notifies lasso weapon to snap loop onto target
 */
UCLASS()
class CATTLEGAME_API ALassoProjectile : public AActor
{
	GENERATED_BODY()

public:
	ALassoProjectile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== PROJECTILE CONTROL =====

	/** Launch the projectile with initial direction */
	UFUNCTION(BlueprintCallable, Category = "Lasso|Projectile")
	void Launch(const FVector& Direction);

	/** Set owning lasso weapon for callbacks */
	void SetLassoWeapon(ALasso* Weapon) { LassoWeapon = Weapon; }

	UFUNCTION(BlueprintCallable, Category = "Lasso|Projectile")
	AActor* GetAimAssistTarget() const { return AimAssistTarget.Get(); }

	/** Get the visual rope loop mesh */
	UStaticMeshComponent* GetRopeLoopMesh() const { return RopeLoopMesh; }

protected:
	// ===== COMPONENTS =====

	/** Small collision sphere for hit detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> HitSphere;

	/** Visual mesh for the flying rope loop (No Collision) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RopeLoopMesh;

	/** Projectile movement with gravity arc */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// ===== CONFIGURATION =====

	/** Initial throw speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float InitialSpeed = 2500.0f;

	/** Gravity scale for arc (0 = straight, 1 = full gravity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float GravityScale = 0.5f;

	/** Max flight time before auto-miss */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float MaxFlightTime = 1.5f;

	/** Aim assist detection radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|AimAssist")
	float AimAssistRadius = 200.0f;

	/** Aim assist cone half-angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|AimAssist")
	float AimAssistAngle = 30.0f;

	/** How fast to lerp toward aim assist target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|AimAssist")
	float AimAssistLerpSpeed = 8.0f;

	/** Tag for valid lasso targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	FName LassoableTag = FName("Target.Lassoable");

	// ===== STATE =====

	/** Reference to owning lasso weapon */
	UPROPERTY()
	TObjectPtr<ALasso> LassoWeapon;

	/** Current aim assist target */
	TWeakObjectPtr<AActor> AimAssistTarget;

	/** Flight timer */
	float FlightTime = 0.0f;

	/** Has already hit something */
	/** Has already hit something */
	/** Has already hit something */
	bool bHasHit = false;

private:
	/** Handle collision with actors */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Handle overlap (for aim assist detection) */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Update aim assist - find and track best target */
	void UpdateAimAssist(float DeltaTime);

	/** Check if actor is a valid lasso target */
	bool IsValidTarget(AActor* Actor) const;

	/** Notify weapon of hit or miss */
	void OnTargetHit(AActor* Target);
	void OnTargetMissed();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
