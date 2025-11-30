#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "HitscanWeaponBase.generated.h"

class USkeletalMeshComponent;
class ACattleCharacter;

/**
 * Base for hitscan weapons. Provides a lightweight client prediction helper
 * that triggers cosmetic feedback immediately on the client and sends a
 * server RPC with start/dir for authoritative processing.
 */
UCLASS(Abstract)
class CATTLEGAME_API AHitscanWeaponBase : public AWeaponBase
{
    GENERATED_BODY()

public:
    AHitscanWeaponBase() = default;

protected:
    // Client-side entry: request a server fire while playing predicted cosmetics
    void RequestServerFireWithPrediction(const FVector &TraceStart, const FVector &TraceDir);

    // Override in derived classes to implement authoritative trace/damage on server
    virtual void OnServerFire(const FVector &TraceStart, const FVector &TraceDir) PURE_VIRTUAL(AHitscanWeaponBase::OnServerFire, );

    // Optional blueprint cosmetic hook for prediction
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Hitscan")
    void OnPredictedHitscanFired(ACattleCharacter *Character, USkeletalMeshComponent *Mesh, FVector TraceStart, FVector TraceDir);

private:
    UFUNCTION(Server, Reliable)
    void ServerFire(const FVector_NetQuantize &TraceStart, const FVector_NetQuantizeNormal &TraceDir);
    void ServerFire_Implementation(const FVector_NetQuantize &TraceStart, const FVector_NetQuantizeNormal &TraceDir);
};
