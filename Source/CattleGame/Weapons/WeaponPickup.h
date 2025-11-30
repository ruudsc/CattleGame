#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponPickup.generated.h"

class AWeaponBase;
class ACattleCharacter;

// Delegate declaration for pickup events
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponPickedUp, ACattleCharacter*);

/**
 * Pickup actor that spawns a weapon when overlapped by a player.
 * Automatically destroys itself after weapon is picked up.
 */
UCLASS()
class CATTLEGAME_API AWeaponPickup : public AActor
{
	GENERATED_BODY()

public:
	AWeaponPickup();

	// ===== PICKUP CONFIGURATION =====

	/** Class of weapon to spawn when picked up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Weapon")
	TSubclassOf<AWeaponBase> WeaponClass;

	/** Whether to spawn weapon immediately or wait for overlap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Behavior")
	bool bSpawnWeaponOnBeginPlay = false;

	/** Visual static mesh for the pickup */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Visual")
	class UStaticMeshComponent* PickupMesh = nullptr;

	/** Sphere collision for pickup trigger */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Collision")
	class USphereComponent* TriggerSphere = nullptr;

	/** Rotation speed for visual mesh spin effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Visual")
	float RotationSpeed = 90.0f;

	/** Height bob speed for visual mesh animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Visual")
	float BobSpeed = 2.0f;

	/** Height bob amplitude in units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Visual")
	float BobHeight = 20.0f;

	// ===== PICKUP EVENTS =====

	/** Called when weapon is successfully picked up */
	FOnWeaponPickedUp OnWeaponPickedUp;

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	// ===== COLLISION EVENTS =====

	UFUNCTION()
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:
	/** The spawned weapon instance */
	UPROPERTY()
	AWeaponBase* SpawnedWeapon = nullptr;

	/** Original location for bobbing animation */
	FVector OriginalLocation = FVector::ZeroVector;

	/** Time tracker for animations */
	float AnimationTime = 0.0f;

	/**
	 * Spawn the weapon actor.
	 */
	void SpawnWeapon();

	/**
	 * Try to give weapon to overlapping player.
	 */
	void AttemptPickup(ACattleCharacter* Character);

	/**
	 * Destroy this pickup actor.
	 */
	void DestroyPickup();
};
