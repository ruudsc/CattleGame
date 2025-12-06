#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "ProjectileWeaponBase.generated.h"

class USkeletalMeshComponent;
class ACattleCharacter;

/**
 * Base for projectile/throwable weapons. Exposes a small client prediction helper
 * that plays cosmetics on the client and sends the authoritative fire request
 * to the server with a start/dir payload.
 */
UCLASS(Abstract)
class CATTLEGAME_API AProjectileWeaponBase : public AWeaponBase
{
    GENERATED_BODY()

public:
    AProjectileWeaponBase() = default;

    /**
     * Public method to fire the projectile weapon with client-side prediction.
     * Called by abilities to initiate the throw/fire sequence.
     */
    UFUNCTION(BlueprintCallable, Category = "Weapon|Projectile")
    void FireWithPrediction(const FVector &SpawnLocation, const FVector &LaunchDirection);

protected:
    // Client-side entry: request a server fire while playing predicted cosmetics
    void RequestServerFireWithPrediction(const FVector &SpawnLocation, const FVector &LaunchDirection);

    // Override in derived classes to implement authoritative spawn/launch on server
    virtual void OnServerFire(const FVector &SpawnLocation, const FVector &LaunchDirection) PURE_VIRTUAL(AProjectileWeaponBase::OnServerFire, );

    // Optional blueprint cosmetic hook for prediction
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Projectile")
    void OnPredictedProjectileFired(ACattleCharacter *Character, USkeletalMeshComponent *Mesh, FVector SpawnLocation, FVector LaunchDirection);

private:
    UFUNCTION(Server, Reliable)
    void ServerFire(const FVector_NetQuantize &SpawnLocation, const FVector_NetQuantizeNormal &LaunchDirection);
    void ServerFire_Implementation(const FVector_NetQuantize &SpawnLocation, const FVector_NetQuantizeNormal &LaunchDirection);
};
