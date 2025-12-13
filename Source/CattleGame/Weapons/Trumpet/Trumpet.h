#pragma once

#include "CoreMinimal.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "Delegates/Delegate.h"
#include "Trumpet.generated.h"

class UStaticMeshComponent;
class ACattleAnimal;

/**
 * Trumpet Weapon - Sound-based effects with dual abilities.
 *
 * Mechanics:
 * - Primary Fire: Lure ability - calms cattle, then attracts when calm enough
 * - Secondary Fire: Scare ability - adds fear to cattle
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

	virtual void Tick(float DeltaTime) override;

	// ===== WEAPON INTERFACE =====

	/** Check if trumpet can play (always true) */
	virtual bool CanFire() const;

	/** Play Lure ability (primary fire) */
	UFUNCTION(BlueprintCallable, Category = "Trumpet")
	void PlayLure();

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
	virtual void BeginPlay() override;

	// ===== STATE =====

	/** Whether trumpet is currently playing */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trumpet|State")
	bool bIsPlaying = false;

	/** Whether playing Lure (true) or Scare (false) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trumpet|State")
	bool bIsPlayingLure = false;

	// ===== LURE PROPERTIES =====

	/** Radius of Lure effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Lure")
	float LureRadius = 1500.0f;

	/** Calm applied per second while luring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Lure")
	float CalmPerSecond = 25.0f;

	/** Fear percentage threshold below which cattle start being attracted (0.0-1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Lure", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LureAttractionThreshold = 0.3f;

	/** Speed at which lured cattle move toward player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Lure")
	float LureAttractionSpeed = 200.0f;

	// ===== SCARE PROPERTIES =====

	/** Radius of Scare effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Scare")
	float ScareRadius = 1500.0f;

	/** Fear applied per second while scaring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trumpet|Scare")
	float FearPerSecond = 40.0f;

	// ===== NETWORK =====

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
	/** Apply lure effects to nearby cattle */
	void ApplyLureEffects(float DeltaTime);

	/** Apply scare effects to nearby cattle */
	void ApplyScareEffects(float DeltaTime);

	/** Get all cattle within a radius using sphere overlap */
	TArray<ACattleAnimal *> GetCattleInRadius(float Radius) const;
};
