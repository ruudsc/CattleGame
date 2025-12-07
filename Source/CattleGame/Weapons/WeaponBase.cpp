#include "WeaponBase.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "Net/UnrealNetwork.h"
#include "CattleGame/CattleGame.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.TickInterval = 0.1f;
	bReplicates = true;
	bNetLoadOnClient = true;

	// No root component - weapon is purely logical, mesh is handled in blueprints
	SetRootComponent(nullptr);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Initially hide weapons that aren't equipped
	SetActorHiddenInGame(!bIsEquipped);
}

void AWeaponBase::OnRep_IsEquipped()
{
	// When bIsEquipped replicates to clients, update visibility
	SetActorHiddenInGame(!bIsEquipped);

	UE_LOG(LogGASDebug, Warning, TEXT("%s::OnRep_IsEquipped - bIsEquipped=%s, Hidden=%s"),
		   *GetName(), bIsEquipped ? TEXT("TRUE") : TEXT("FALSE"), IsHidden() ? TEXT("TRUE") : TEXT("FALSE"));

	// If equipped and not yet attached, attach to character
	if (bIsEquipped && !GetAttachParentActor())
	{
		AttachToCharacterHand();
	}
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

ACattleCharacter *AWeaponBase::GetOwnerCharacter() const
{
	return OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
}

FTransform AWeaponBase::GetAttachmentOffsetForMesh(const USkeletalMeshComponent *Mesh) const
{
	if (!Mesh)
	{
		return FirstPersonAttachmentOffset;
	}

	const ACattleCharacter *CurrentOwner = GetOwnerCharacter();
	if (CurrentOwner)
	{
		if (Mesh == CurrentOwner->GetFirstPersonMesh())
		{
			return FirstPersonAttachmentOffset;
		}

		if (Mesh == CurrentOwner->GetMesh())
		{
			return ThirdPersonAttachmentOffset;
		}
	}

	// Fallback: use first-person offset if the mesh is unknown
	return FirstPersonAttachmentOffset;
}

void AWeaponBase::SetOwnerCharacter(ACattleCharacter *Character)
{
	OwnerCharacter = Character;
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate owner character reference
	DOREPLIFETIME(AWeaponBase, OwnerCharacter);
	DOREPLIFETIME(AWeaponBase, bIsEquipped);
}

bool AWeaponBase::IsNetRelevantFor(const AActor *RealViewer, const AActor *Viewer, const FVector &SrcLocation) const
{
	// Weapons are only relevant to their owner character and nearby players
	if (OwnerCharacter && Viewer == OwnerCharacter)
	{
		return true;
	}

	// Also relevant to nearby pawns (other players within view distance)
	return Super::IsNetRelevantFor(RealViewer, Viewer, SrcLocation);
}

USkeletalMeshComponent *AWeaponBase::GetWeaponMesh() const
{
	return WeaponMesh;
}

void AWeaponBase::AttachToCharacterHand()
{
	ACattleCharacter *CurrentOwner = GetOwnerCharacter();
	if (!CurrentOwner)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("%s::AttachToCharacterHand - No owner character"), *GetName());
		return;
	}

	USkeletalMeshComponent *TargetMesh = CurrentOwner->GetActiveCharacterMesh();
	if (!TargetMesh)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("%s::AttachToCharacterHand - No active character mesh"), *GetName());
		return;
	}

	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
	AttachToComponent(TargetMesh, AttachRules, AttachmentSocketName);

	UE_LOG(LogGASDebug, Log, TEXT("%s::AttachToCharacterHand - Attached to socket %s on mesh %s"), *GetName(), *AttachmentSocketName.ToString(), *TargetMesh->GetName());
}

// Intentionally no base CanFire/CanReload implementation; derived weapons decide their own gating
