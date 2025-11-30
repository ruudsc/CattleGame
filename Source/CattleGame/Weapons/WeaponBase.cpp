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
	// All initialization is handled in child classes
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeaponBase::EquipWeapon()
{
	SetActorHiddenInGame(false);

	// Notify blueprints to handle all equip logic
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;

	UE_LOG(LogGASDebug, Warning, TEXT("WeaponBase::EquipWeapon() called on %s"), *GetName());

	OnWeaponEquipped(CurrentOwner, ActiveMesh);
}

void AWeaponBase::UnequipWeapon()
{
	SetActorHiddenInGame(true);

	// Notify blueprints to handle all unequip logic
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
	OnWeaponUnequipped(CurrentOwner, ActiveMesh);
}

void AWeaponBase::Fire()
{
	// Notify blueprints to handle all firing logic
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
	OnWeaponFired(CurrentOwner, ActiveMesh);
}

void AWeaponBase::Reload()
{
	// Notify blueprints to handle all reload logic
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
	OnReloadStarted(CurrentOwner, ActiveMesh);
}

FTransform AWeaponBase::GetAttachmentOffsetForMesh(const USkeletalMeshComponent *Mesh) const
{
	if (!Mesh)
	{
		return FirstPersonAttachmentOffset;
	}

	const ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
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
	// Get the character owner
	const ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	if (!CurrentOwner)
	{
		return nullptr;
	}

	// Return the active mesh (first-person for owner, third-person for others)
	return CurrentOwner->GetActiveCharacterMesh();
}

// Intentionally no base CanFire/CanReload implementation; derived weapons decide their own gating
