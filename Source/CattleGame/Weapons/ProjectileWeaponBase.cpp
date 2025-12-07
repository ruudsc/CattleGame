#include "ProjectileWeaponBase.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void AProjectileWeaponBase::FireWithPrediction(const FVector &SpawnLocation, const FVector &LaunchDirection)
{
    RequestServerFireWithPrediction(SpawnLocation, LaunchDirection);
}

void AProjectileWeaponBase::RequestServerFireWithPrediction(const FVector &SpawnLocation, const FVector &LaunchDirection)
{
    // Cosmetic prediction: let client play effects immediately
    ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
    USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
    OnPredictedProjectileFired(CurrentOwner, ActiveMesh, SpawnLocation, LaunchDirection);

    // Always ask server to do authority work
    ServerFire(SpawnLocation, LaunchDirection.GetSafeNormal());
}

void AProjectileWeaponBase::ServerFire_Implementation(const FVector_NetQuantize &SpawnLocation, const FVector_NetQuantizeNormal &LaunchDirection)
{
    OnServerFire(SpawnLocation, LaunchDirection);
}
