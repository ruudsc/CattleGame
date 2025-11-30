#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LassoProjectile.generated.h"

class ALasso;
class USphereComponent;
class UProjectileMovementComponent;
class UCableComponent;

/**
 * Lasso Projectile - Flying rope loop that arcs toward targets.
 *
 * Features:
 * - Gravity arc (like a thrown rope)
 * - Aim assist cone for nearby targets
 * - Auto-retract on max range or missed
 * - Cable component for visual rope
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

	/** Launch the projectile with direction and speed */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Launch(const FVector &Direction, float Speed);

	/** Set lasso properties from weapon */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetLassoProperties(float Speed, float MaxRange, float Gravity, float AimAssist, float AimAssistRange, float Retract);

	/** Set reference to weapon for callbacks */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetLassoWeapon(ALasso *Weapon);

	/** Start retracting back to owner */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void StartRetract();

	// ===== STATE QUERIES =====

	/** Check if projectile is retracting */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	bool IsRetracting() const { return bIsRetracting; }

	/** Get the cable component for rope visuals */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	UCableComponent *GetCableComponent() const { return CableComponent; }

protected:
	// ===== COMPONENTS =====

	/** Root collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent *CollisionSphere;

	/** Mesh component for rope loop visualization */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent *MeshComponent;

	/** Cable component for rope visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCableComponent *CableComponent;

	/** Movement component for projectile physics */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent *ProjectileMovement;

	// ===== LASSO PROPERTIES =====

	/** Speed of lasso projectile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso")
	float LassoSpeed = 1500.0f;

	/** Max range before lasso fails to hook */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso")
	float LassoRange = 1000.0f;

	/** Gravity scale for arc trajectory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso")
	float GravityScale = 0.5f;

	/** Aim assist cone angle (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso")
	float AimAssistAngle = 15.0f;

	/** Aim assist max range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso")
	float AimAssistMaxDistance = 800.0f;

	/** Retract speed when returning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso")
	float RetractSpeed = 2000.0f;

	// ===== STATE =====

	/** Whether projectile is retracting back to owner */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	bool bIsRetracting = false;

	/** Whether projectile has caught a target (prevents multiple catches) */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bHasCaughtTarget = false;

	/** Reference back to weapon for state updates */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	ALasso *LassoWeapon = nullptr;

	/** Initial owner position (for range checking) */
	FVector OwnerInitialLocation = FVector::ZeroVector;

	/** Distance traveled so far */
	float DistanceTraveled = 0.0f;

	/** Current aim assist target (if any) */
	TWeakObjectPtr<AActor> AimAssistTarget;

private:
	/** Handle collision with world and targets */
	UFUNCTION()
	void OnCollision(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
					 FVector NormalImpulse, const FHitResult &Hit);

	/** Handle overlap start (for aim assist) */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

	/** Check for aim assist targets in cone */
	void UpdateAimAssist();

	/** Move toward retract target */
	void TickRetract(float DeltaTime);

	/** Get lifetime replicated properties */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
