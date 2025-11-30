#include "Dynamite.h"
#include "DynamiteProjectile.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "CattleGame/CattleGame.h"

ADynamite::ADynamite()
{
	WeaponSlotID = 2; // Dynamite is in slot 2
	WeaponName = FString(TEXT("Dynamite"));
	bReplicates = true;

	// Create scene root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Create dynamite mesh component and attach to root
	DynamiteMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DynamiteMesh"));
	DynamiteMeshComponent->SetupAttachment(RootComponent);
	DynamiteMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADynamite::EquipWeapon()
{
	// Call base to fire event
	Super::EquipWeapon();

	// Attach to character hand socket
	AttachToCharacterHand();

	UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::EquipWeapon - Attached to character hand"));
}

void ADynamite::UnequipWeapon()
{
	// Detach from character
	// DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Call base to fire event
	Super::UnequipWeapon();

	UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::UnequipWeapon - Detached from character"));
}

void ADynamite::Fire()
{
	// Trigger blueprint event for local cosmetics
	Super::Fire();

	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	if (!CurrentOwner || !CanFire())
	{
		return;
	}

	const FVector SpawnLocation = CurrentOwner->GetActorLocation() + CurrentOwner->GetActorForwardVector() * 100.0f;
	const FVector LaunchDirection = CurrentOwner->GetControlRotation().Vector();

	// Client-predicted spawn request
	RequestServerFireWithPrediction(SpawnLocation, LaunchDirection);
}

bool ADynamite::CanFire() const
{
	// Can fire if we have ammo and weapon is equipped
	if (CurrentAmmo <= 0)
	{
		return false;
	}

	return true;
}

void ADynamite::Reload()
{
	// For dynamite, reload just fills ammo to max
	Super::Reload();

	if (HasAuthority())
	{
		CurrentAmmo = MaxAmmo;
		UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::Reload - Ammo refilled to %d"), MaxAmmo);
	}
}

bool ADynamite::CanReload() const
{
	// Can reload if ammo is not full
	if (CurrentAmmo >= MaxAmmo)
	{
		return false;
	}

	return true;
}

void ADynamite::AddAmmo(int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentAmmo = FMath::Min(CurrentAmmo + Amount, MaxAmmo);
	UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::AddAmmo - Added %d ammo, total: %d/%d"),
		   Amount, CurrentAmmo, MaxAmmo);
}

void ADynamite::SpawnDynamiteProjectile()
{
	if (!GetWorld() || !ProjectileClass || !OwnerCharacter)
	{
		UE_LOG(LogGASDebug, Error, TEXT("Dynamite::SpawnDynamiteProjectile - Missing world, projectile class, or owner"));
		return;
	}

	// Get spawn location from weapon muzzle (approximate - camera forward for now)
	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 100.0f;
	FRotator SpawnRotation = OwnerCharacter->GetControlRotation();

	// Spawn the projectile
	ADynamiteProjectile *Projectile = GetWorld()->SpawnActor<ADynamiteProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation);

	if (Projectile)
	{
		// Initialize projectile properties
		Projectile->SetOwner(OwnerCharacter);
		Projectile->SetExplosionProperties(ExplosionRadius, ExplosionDamage);
		Projectile->SetFuseTime(FuseTime);

		// Apply throwing force
		FVector ThrowDirection = SpawnRotation.Vector();
		Projectile->Launch(ThrowDirection, ThrowForce);

		UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::SpawnDynamiteProjectile - Projectile spawned at %s with force %f"),
			   *SpawnLocation.ToString(), ThrowForce);
	}
	else
	{
		UE_LOG(LogGASDebug, Error, TEXT("Dynamite::SpawnDynamiteProjectile - Failed to spawn projectile"));
	}
}

void ADynamite::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADynamite, CurrentAmmo);
}

void ADynamite::OnServerFire(const FVector &SpawnLocation, const FVector &LaunchDirection)
{
	if (!GetWorld() || !ProjectileClass || !OwnerCharacter)
	{
		UE_LOG(LogGASDebug, Error, TEXT("Dynamite::OnServerFire - Missing world, projectile class, or owner"));
		return;
	}

	if (!CanFire())
	{
		return;
	}

	ADynamiteProjectile *Projectile = GetWorld()->SpawnActor<ADynamiteProjectile>(
		ProjectileClass,
		SpawnLocation,
		LaunchDirection.Rotation());

	if (Projectile)
	{
		Projectile->SetOwner(OwnerCharacter);
		Projectile->SetExplosionProperties(ExplosionRadius, ExplosionDamage);
		Projectile->SetFuseTime(FuseTime);
		Projectile->Launch(LaunchDirection.GetSafeNormal(), ThrowForce);

		// Consume ammo on server
		--CurrentAmmo;

		UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::OnServerFire - Projectile spawned at %s, ammo: %d/%d"), *SpawnLocation.ToString(), CurrentAmmo, MaxAmmo);
	}
	else
	{
		UE_LOG(LogGASDebug, Error, TEXT("Dynamite::OnServerFire - Failed to spawn projectile"));
	}
}

void ADynamite::AttachToCharacterHand()
{
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	if (!CurrentOwner)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::AttachToCharacterHand - No owner character"));
		return;
	}

	USkeletalMeshComponent *TargetMesh = CurrentOwner->GetActiveCharacterMesh();
	if (!TargetMesh)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Dynamite::AttachToCharacterHand - No active character mesh"));
		return;
	}

	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
	AttachToComponent(TargetMesh, AttachRules, DynamiteHandSocketName);

	UE_LOG(LogGASDebug, Log, TEXT("Dynamite::AttachToCharacterHand - Attached to socket %s on mesh %s"), *DynamiteHandSocketName.ToString(), *TargetMesh->GetName());
}
