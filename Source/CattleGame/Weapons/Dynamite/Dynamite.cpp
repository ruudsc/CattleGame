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

bool ADynamite::CanFire() const
{
	// Can fire if we have ammo and weapon is equipped
	if (CurrentAmmo <= 0)
	{
		return false;
	}

	return true;
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
