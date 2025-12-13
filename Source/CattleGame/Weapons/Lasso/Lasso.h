#pragma once

#include "CoreMinimal.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "Lasso.generated.h"

class ALassoProjectile;
class UCableComponent;
class ULassoableComponent;

/**
 * Lasso State - Tracks weapon state machine
 */
UENUM(BlueprintType)
enum class ELassoState : uint8
{
	/** Ready to throw */
	Idle UMETA(DisplayName = "Idle"),

	/** Projectile in flight (invisible arc) */
	Throwing UMETA(DisplayName = "Throwing"),

	/** Target caught, constraint active */
	Tethered UMETA(DisplayName = "Tethered"),

	/** Rope returning to player */
	Retracting UMETA(DisplayName = "Retracting")
};

/**
 * Lasso Weapon - RDR2-style implementation.
 *
 * Architecture:
 * - Invisible projectile arc for hit detection
 * - Snap-on capture (no physics rope simulation)
 * - Simple distance constraint (stable, no explosions)
 * - UCableComponent for visual rope
 *
 * State Machine:
 * - IDLE: Ready to throw. Fire → THROWING
 * - THROWING: Arc projectile in flight. Hit → TETHERED, Miss → RETRACTING
 * - TETHERED: Target caught. Hold fire to pull. Release/Secondary → RETRACTING
 * - RETRACTING: Visual cleanup. Complete → IDLE
 */
UCLASS()
class CATTLEGAME_API ALasso : public AWeaponBase
{
	GENERATED_BODY()

public:
	ALasso();

	virtual void Tick(float DeltaTime) override;

	// ===== WEAPON INTERFACE =====

	/** Can the lasso be thrown? */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	bool CanFire() const;

	/** Start pulling the tethered target */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void StartPulling();

	/** Stop pulling the tethered target */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void StopPulling();

	/** Release the tethered target */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void ReleaseTether();

	/** Force reset to idle state */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void ForceReset();

	// ===== STATE QUERIES =====

	/** Get current state */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	ELassoState GetLassoState() const { return CurrentState; }

	/** Get tethered target */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	AActor *GetTetheredTarget() const { return TetheredTarget; }

	/** Is currently pulling? */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	bool IsPulling() const { return bIsPulling; }

	// ===== PROJECTILE CALLBACKS =====

	/** Called by projectile on hit */
	void OnProjectileHitTarget(AActor *Target);

	/** Called by projectile on miss */
	void OnProjectileMissed();

	// ===== BLUEPRINT EVENTS =====

	/** Called when lasso is thrown */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoThrown();

	/** Called when target is caught */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnTargetCaptured(AActor *Target);

	/** Called when target is released */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnTargetReleased();

	/** Called when retract completes */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnRetractComplete();

	// ===== SERVER RPCS =====

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lasso")
	void ServerFire(const FVector_NetQuantize &SpawnLocation, const FVector_NetQuantizeNormal &LaunchDirection);

	UFUNCTION(Server, Reliable)
	void ServerReleaseTether();

protected:
	// ===== STATE =====

	/** Current state */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly, Category = "Lasso|State")
	ELassoState CurrentState = ELassoState::Idle;

	/** Tethered target actor */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lasso|State")
	TObjectPtr<AActor> TetheredTarget;

	/** Is player holding pull input? */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	bool bIsPulling = false;

	/** Current constraint length (set on capture) */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	float ConstraintLength = 0.0f;

	// ===== PROJECTILE =====

	/** Active projectile instance */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|Projectile")
	TObjectPtr<ALassoProjectile> ActiveProjectile;

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	TSubclassOf<ALassoProjectile> ProjectileClass;

	// ===== VISUAL =====

	/** Cable component for rope visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lasso|Visual")
	TObjectPtr<UCableComponent> RopeCable;

	/** Static mesh for lasso coil in hand (visible during throw) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lasso|Visual")
	TObjectPtr<UStaticMeshComponent> HandCoilMesh;

public:
	/** Get the hand coil mesh component */
	UStaticMeshComponent *GetHandCoilMesh() const { return HandCoilMesh; }

protected:
	/** Spawned loop mesh actor on target */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|Visual")
	TObjectPtr<AActor> SpawnedLoopMesh;

	/** Class for loop mesh to spawn on target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	TSubclassOf<AActor> LoopMeshClass;

	// ===== CONFIGURATION =====

	/** Maximum constraint length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float MaxConstraintLength = 1200.0f;

	/** Pull force applied when constraint is stretched */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float PullForce = 2000.0f;

	/** How fast the constraint shortens while pulling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float PullReelSpeed = 200.0f;

	/** Cooldown after retract before next throw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float ThrowCooldown = 0.5f;

	/** Retract animation duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float RetractDuration = 0.5f;

	// ===== INTERNAL =====

	float CooldownRemaining = 0.0f;
	float RetractTimer = 0.0f;

	/** Counter to throttle debug logging (logs every N ticks instead of every tick) */
	int32 LassoTickLogCounter = 0;

	// ===== NETWORK =====

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentState();

private:
	/** Transition to new state */
	void SetState(ELassoState NewState);

	/** Tick handlers per state */
	void TickThrowing(float DeltaTime);
	void TickTethered(float DeltaTime);
	void TickRetracting(float DeltaTime);
	void TickCooldown(float DeltaTime);

	/** Apply distance constraint force */
	void ApplyConstraintForce(float DeltaTime);

	/** Update cable visual endpoints */
	void UpdateCableVisual();

	/** Spawn projectile */
	void SpawnProjectile(const FVector &Location, const FVector &Direction);

	/** Destroy projectile */
	void DestroyProjectile();

	/** Spawn loop mesh on target */
	void SpawnLoopMeshOnTarget(AActor *Target);

	/** Destroy spawned loop mesh */
	void DestroyLoopMesh();
};
