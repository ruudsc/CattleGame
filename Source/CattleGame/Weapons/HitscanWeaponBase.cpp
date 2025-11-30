#include "HitscanWeaponBase.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void AHitscanWeaponBase::RequestServerFireWithPrediction(const FVector &TraceStart, const FVector &TraceDir)
{
    // Cosmetic prediction: let client play effects immediately
    ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
    USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
    OnPredictedHitscanFired(CurrentOwner, ActiveMesh, TraceStart, TraceDir);

    // Always ask server to do authority work
    ServerFire(TraceStart, TraceDir);
}

void AHitscanWeaponBase::ServerFire_Implementation(const FVector_NetQuantize &TraceStart, const FVector_NetQuantizeNormal &TraceDir)
{
    OnServerFire(TraceStart, TraceDir);
}
