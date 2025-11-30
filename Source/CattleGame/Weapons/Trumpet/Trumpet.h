#pragma once

#include "CoreMinimal.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "Delegates/Delegate.h"
#include "Trumpet.generated.h"

class UStaticMeshComponent;

/**
 * Trumpet Weapon - Sound-based effects with dual abilities.
 *
 * Mechanics:
 * - Primary Fire: Lure ability - attracts enemies toward player
 * - Secondary Fire: Scare ability - repels enemies away from player
 * - Hold either button to maintain effect
 * - No ammo, unlimited use
 * - Can switch between abilities while playing
 */
UCLASS()
class CATTLEGAME_API ATrumpet : public AWeaponBase
{
	GENERATED_BODY()

public:
	ATrumpet();

	// ===== SOCKET CONSTANTS =====

	/** Socket name for trumpet attachment on character skeleton */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Attachment")
	FName TrumpetHandSocketName = FName(TEXT("HandGrip_R"));

	// ===== WEAPON INTERFACE =====

	/** Override equip to attach to character hand */
	virtual void EquipWeapon() override;

	/** Override unequip to detach from character */
	virtual void UnequipWeapon() override;

	// ===== WEAPON INTERFACE =====

	/** Override Fire for Lure ability */
	virtual void Fire() override;

	/** Check if trumpet can play (always true) */
	virtual bool CanFire() const;

	/** Play Scare ability (secondary fire) */
	UFUNCTION(BlueprintCallable, Category = "Trumpet")
	void PlayScare();

	/** Stop currently playing effect */
	UFUNCTION(BlueprintCallable, Category = "Trumpet")
	void StopPlaying();

	// ===== TRUMPET STATE =====

	/** Check if trumpet is currently playing */
	UFUNCTION(BlueprintCallable, Category = "Trumpet")
	bool IsPlaying() const { return bIsPlaying; }

	/** Get current playing mode (Lure or Scare) */
	UFUNCTION(BlueprintCallable, Category = "Trumpet")
	bool IsPlayingLure() const { return bIsPlayingLure; }

	// ===== CALLBACKS =====

	/** Called when trumpet starts playing */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTrumpetStartedDelegate);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FTrumpetStartedDelegate OnTrumpetStarted;

	/** Called when trumpet stops playing */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTrumpetStoppedDelegate);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FTrumpetStoppedDelegate OnTrumpetStopped;

	// ===== MESH =====

	/** Static mesh component for the trumpet visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trumpet|Visual")
	UStaticMeshComponent *TrumpetMeshComponent = nullptr;

protected:
	// ===== PROPERTIES =====

	/** Whether trumpet is currently playing */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trumpet|State")
	bool bIsPlaying = false;

	/** Whether playing Lure (true) or Scare (false) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trumpet|State")
	bool bIsPlayingLure = false;

	// ===== LURE PROPERTIES =====

	/** Radius of Lure effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Lure")
	float LureRadius = 800.0f;

	/** Strength of Lure attraction (force applied to targets) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Lure")
	float LureStrength = 500.0f;

	// ===== SCARE PROPERTIES =====

	/** Radius of Scare effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Scare")
	float ScareRadius = 800.0f;

	/** Strength of Scare repulsion (force applied to targets) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Scare")
	float ScareStrength = 500.0f;

	// ===== NETWORK =====

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
	/** Start playing Lure effect */
	void PlayLure();

	/** Get all targets affected by current effect */
	void GetAffectedTargets(TArray<AActor *> &OutTargets);

	/** Apply lure or scare force to targets */
	void ApplyEffectForces();

	/** Attach trumpet mesh to character hand socket */
	void AttachToCharacterHand();
};
