#pragma once

#include "CoreMinimal.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "Delegates/Delegate.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "Lasso.generated.h"

class ALassoProjectile;
class UAbilitySystemComponent;

/**
 * Lasso State Enum - Tracks the current state of the lasso weapon
 */
UENUM(BlueprintType)
enum class ELassoState : uint8
{
	/** Lasso is ready to be thrown */
	Idle UMETA(DisplayName = "Idle"),

	/** Projectile is in flight, not yet hit anything */
	InFlight UMETA(DisplayName = "In Flight"),

	/** Projectile has caught a target, rope is tethered */
	Tethered UMETA(DisplayName = "Tethered"),

	/** Rope is returning to player (after miss, release, or break-free) */
	Retracting UMETA(DisplayName = "Retracting")
};

/**
 * Lasso Weapon - Tether weapon that casts a projectile rope to catch and pull enemies.
 *
 * State Machine:
 * - IDLE: Ready to throw. Primary Fire → IN_FLIGHT
 * - IN_FLIGHT: Projectile flying. Hit valid target → TETHERED. Miss/max range → RETRACTING
 * - TETHERED: Target caught. Hold Primary → Pull. Secondary Fire → RETRACTING
 * - RETRACTING: Rope returning. Complete → IDLE (after cooldown)
 *
 * Mechanics:
 * - Fire: Cast lasso projectile with gravity arc and slight aim assist
 * - Hit: If projectile hits valid target (animal/player), enter TETHERED state
 * - Hold Fire: While tethered, applies elastic pull force to both player and target
 * - Secondary Fire: Release tethered target, enter RETRACTING state
 * - Elastic Rope: Spring physics pulls both entities toward each other based on mass
 * - Auto-retract: On miss or exceeding max rope length (1000 units)
 * - Cooldown: 0.5s after retract completes before next throw
 */
UCLASS()
class CATTLEGAME_API ALasso : public AWeaponBase
{
	GENERATED_BODY()

public:
	ALasso();

	virtual void Tick(float DeltaTime) override;

	// ===== WEAPON INTERFACE =====

	/** Override Fire to cast the lasso (Primary Fire) */
	virtual void Fire() override;

	/** Check if lasso can be thrown */
	virtual bool CanFire() const;

	/** Release the tethered target and start retracting (Secondary Fire) */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void ReleaseTether();

	/** Start retracting the rope back to player */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void StartRetract();

	// ===== LASSO STATE =====

	/** Get current lasso state */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	ELassoState GetLassoState() const { return CurrentState; }

	/** Check if lasso is in a specific state */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	bool IsInState(ELassoState State) const { return CurrentState == State; }

	/** Get the currently tethered actor (nullptr if not tethered) */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	AActor *GetTetheredTarget() const { return TetheredTarget; }

	/** Check if player is currently holding pull input */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	bool IsPulling() const { return bIsPulling; }

	/** Set pull input state (called by ability) */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	void SetPulling(bool bPulling);

	/** Get remaining cooldown time */
	UFUNCTION(BlueprintCallable, Category = "Lasso|State")
	float GetCooldownRemaining() const { return CooldownRemaining; }

	// ===== PROJECTILE CALLBACKS =====

	/** Called by projectile when it hits a valid target */
	void OnProjectileHitTarget(AActor *Target);

	/** Called by projectile when it misses (hits world or exceeds range) */
	void OnProjectileMissed();

	/** Called when retract is complete (projectile reached player) */
	void OnRetractComplete();

	/** Get the lasso projectile instance */
	UFUNCTION(BlueprintCallable, Category = "Lasso|Projectile")
	ALassoProjectile *GetProjectile() const { return LassoProjectile; }

	// ===== PHYSICS HELPERS =====

	/** Get mass of an actor for physics calculations */
	UFUNCTION(BlueprintCallable, Category = "Lasso|Physics")
	static float GetActorMass(AActor *Actor);

	/** Helper to add/remove gameplay tags from character's ability system */
	void UpdateCharacterAbilityTag(AActor *Character, const FGameplayTag &Tag, bool bAdd);

	// ===== WEAPON INTERFACE OVERRIDES =====

	/** Override equip to attach to character hand */
	virtual void EquipWeapon() override;

	/** Override unequip to detach from character */
	virtual void UnequipWeapon() override;

	// ===== BLUEPRINT EVENTS =====

	/** Called when lasso projectile is fired */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoThrown();
	virtual void OnLassoThrown_Implementation();

	/** Called when lasso catches a target */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoCaughtTarget(AActor *CaughtTarget);
	virtual void OnLassoCaughtTarget_Implementation(AActor *CaughtTarget);

	/** Called when tethered target is released */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoReleased();
	virtual void OnLassoReleased_Implementation();

	/** Called when lasso starts retracting */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoRetractStarted();
	virtual void OnLassoRetractStarted_Implementation();

	/** Called when lasso finishes retracting and enters cooldown */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoRetractComplete();
	virtual void OnLassoRetractComplete_Implementation();

	/** Called when cooldown ends and lasso is ready again */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoReady();
	virtual void OnLassoReady_Implementation();

	// ===== VISUAL EVENTS (Phase 4) =====

	/** Called to show rope wrap visual on tethered target. Blueprint should spawn wrap mesh/decal. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Visual")
	void OnShowRopeWrapVisual(AActor *Target);
	virtual void OnShowRopeWrapVisual_Implementation(AActor *Target);

	/** Called to hide rope wrap visual when target is released. Blueprint should remove wrap mesh/decal. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Visual")
	void OnHideRopeWrapVisual(AActor *Target);
	virtual void OnHideRopeWrapVisual_Implementation(AActor *Target);

	// ===== STATE CHANGE EVENTS =====

	/** Called when lasso state changes. Useful for complex Blueprint logic that needs to react to any state transition. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoStateChanged(ELassoState OldState, ELassoState NewState);
	virtual void OnLassoStateChanged_Implementation(ELassoState OldState, ELassoState NewState);

	/** Called when player starts holding pull input while tethered. Blueprint can start tension VFX. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnPullStarted();
	virtual void OnPullStarted_Implementation();

	/** Called when player releases pull input. Blueprint can stop tension VFX. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnPullStopped();
	virtual void OnPullStopped_Implementation();

	/** Called when rope tension changes (while pulling). Stretch is distance beyond resting length, Force is the applied spring force. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnRopeTensionChanged(float Stretch, float Force);
	virtual void OnRopeTensionChanged_Implementation(float Stretch, float Force);

	/** Called when tethered target becomes invalid (destroyed, disconnected). */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnTargetLost(AActor *LostTarget);
	virtual void OnTargetLost_Implementation(AActor *LostTarget);

	/** Called when lasso weapon should be re-attached to character hand socket after retract completes. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnLassoReattached();
	virtual void OnLassoReattached_Implementation();

	/** Called when projectile is destroyed/cleaned up. Blueprint can spawn cleanup VFX. */
	UFUNCTION(BlueprintNativeEvent, Category = "Lasso|Events")
	void OnProjectileDestroyed();
	virtual void OnProjectileDestroyed_Implementation();

	// ===== SOCKET CONSTANTS =====

	/** Socket name for lasso attachment on character skeleton */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Attachment")
	FName LassoHandSocketName = FName(TEXT("HandGrip_R_Lasso"));

	// ===== VISIBILITY =====

	/** Set weapon mesh visibility (hides when thrown, shows when retracted) */
	UFUNCTION(BlueprintCallable, Category = "Lasso|Visual")
	void SetWeaponMeshVisible(bool bVisible);

protected:
	// ===== STATE =====

	/** Current lasso state */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly, Category = "Lasso|State")
	ELassoState CurrentState = ELassoState::Idle;

	/** Currently tethered actor */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lasso|State")
	AActor *TetheredTarget = nullptr;

	/** Is player holding pull input? */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	bool bIsPulling = false;

	/** Remaining cooldown time before next throw */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	float CooldownRemaining = 0.0f;

	/** Current rope length (distance between player and tethered target) */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	float CurrentRopeLength = 0.0f;

	/** Resting rope length (set when target is caught) */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	float RestingRopeLength = 0.0f;

	// ===== PROJECTILE =====

	/** Lasso projectile instance */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|Projectile")
	ALassoProjectile *LassoProjectile = nullptr;

	/** Projectile class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Projectile")
	TSubclassOf<ALassoProjectile> ProjectileClass;

	// ===== MESH =====

	/** Static mesh component for the lasso coil visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lasso|Visual")
	UStaticMeshComponent *LassoMeshComponent = nullptr;

	// ===== CONFIGURATION =====

	/** Maximum rope length (auto-retract distance) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float MaxRopeLength = 1000.0f;

	/** Initial throw speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float ThrowSpeed = 1500.0f;

	/** Gravity scale for arc trajectory (0 = straight, 1 = full gravity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float GravityScale = 0.5f;

	/** Aim assist cone angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float AimAssistAngle = 15.0f;

	/** Maximum distance for aim assist */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float AimAssistMaxDistance = 800.0f;

	/** Rope elasticity (spring constant) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float RopeElasticity = 500.0f;

	/** Rope damping (prevents oscillation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float RopeDamping = 50.0f;

	/** Cooldown after retract completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float ThrowCooldown = 0.5f;

	/** Retract speed (rope return velocity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Config")
	float RetractSpeed = 2000.0f;

	// ===== NETWORK =====

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentState();

private:
	// ===== INTERNAL STATE MANAGEMENT =====

	/** Transition to a new state */
	void SetState(ELassoState NewState);

	/** Attach lasso mesh to character hand socket */
	void AttachToCharacterHand();

	/** Update gameplay tags based on state */
	void UpdateStateTags(ELassoState OldState, ELassoState NewState);

	/** Update weapon visibility based on state (hide when thrown, show when retracted) */
	void UpdateWeaponVisibility(ELassoState OldState, ELassoState NewState);

	// ===== TICK HANDLERS =====

	/** Handle tick while in flight */
	void TickInFlight(float DeltaTime);

	/** Handle tick while tethered */
	void TickTethered(float DeltaTime);

	/** Handle tick while retracting */
	void TickRetracting(float DeltaTime);

	/** Handle cooldown tick */
	void TickCooldown(float DeltaTime);

	// ===== PHYSICS =====

	/** Apply elastic tether force to both player and target */
	void ApplyElasticTetherForce(float DeltaTime);

	// ===== SERVER RPCS =====

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize &SpawnLocation, const FVector_NetQuantizeNormal &LaunchDirection);

	UFUNCTION(Server, Reliable)
	void ServerReleaseTether();

	UFUNCTION(Server, Reliable)
	void ServerStartRetract();
};
