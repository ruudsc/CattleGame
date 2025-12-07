#include "Revolver.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "CattleGame/CattleGame.h"

ARevolver::ARevolver()
{
	// Revolver configuration
	WeaponName = TEXT("Revolver");
	WeaponSlotID = 0;

	// Create scene root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Create revolver mesh component and attach to root
	RevolverMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RevolverMesh"));
	RevolverMeshComponent->SetupAttachment(RootComponent);
	RevolverMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Stats are now defined in header with default values
	MaxAmmo = 6;
	CurrentAmmo = 6;
	DamageAmount = 25.0f;
	FireRate = 2.0f; // 2 shots per second
	ReloadTime = 2.0f;
	WeaponRange = 5000.0f;
	WeaponSpread = 0.0f; // Perfect accuracy
}

void ARevolver::BeginPlay()
{
	Super::BeginPlay();

	// Only initialize on server
	if (!HasAuthority())
	{
		return;
	}

	// Initialize ammo
	CurrentAmmo = MaxAmmo;
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::BeginPlay [SERVER] - %p Ammo initialized to %d/%d"), this, CurrentAmmo, MaxAmmo);
}

bool ARevolver::CanFire() const
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::CanFire() - Addr: %p, HasAuthority: %s, Equipped: %s, Reloading: %s, Ammo: %d/%d"),
		   this,
		   HasAuthority() ? TEXT("TRUE") : TEXT("FALSE"),
		   bIsEquipped ? TEXT("TRUE") : TEXT("FALSE"),
		   bIsReloading ? TEXT("TRUE") : TEXT("FALSE"),
		   CurrentAmmo, MaxAmmo);

	if (!bIsEquipped || bIsReloading)
	{
		UE_LOG(LogGASDebug, Error, TEXT("Revolver::CanFire() - BLOCKED: Equipped=%s Reloading=%s"),
			   bIsEquipped ? TEXT("TRUE") : TEXT("FALSE"),
			   bIsReloading ? TEXT("TRUE") : TEXT("FALSE"));
		return false;
	}

	// Check fire rate
	if (GetWorld())
	{
		const float TimeSinceLastFire = GetWorld()->GetTimeSeconds() - LastFireTime;
		const float FireCooldown = 1.0f / FireRate;
		if (TimeSinceLastFire < FireCooldown)
		{
			const float DisplayTime = FMath::Max(0.0f, TimeSinceLastFire);
			UE_LOG(LogGASDebug, Error, TEXT("Revolver::CanFire() - BLOCKED: Fire rate cooldown (%.2f/%.2f)"), DisplayTime, FireCooldown);
			return false;
		}
	}

	// Check ammo
	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogGASDebug, Error, TEXT("Revolver::CanFire() - BLOCKED: No ammo"));
		return false;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::CanFire() - CAN FIRE!"));

	return true;
}

bool ARevolver::CanReload() const
{
	if (bIsReloading)
	{
		return false;
	}

	// Only reload if ammo is not full
	if (CurrentAmmo < MaxAmmo)
	{
		return true;
	}

	return false;
}

FString ARevolver::GetAmmoString() const
{
	return FString::Printf(TEXT("%d/%d"), CurrentAmmo, MaxAmmo);
}

void ARevolver::OnReloadComplete()
{
	int32 OldAmmo = CurrentAmmo;
	CurrentAmmo = MaxAmmo;
	bIsReloading = false;
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnReloadComplete [%s] - %p Ammo %d->%d, Reloading=false"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), this, OldAmmo, CurrentAmmo);
	if (HasAuthority())
	{
		ForceNetUpdate();
	}

	// OnReloadCompleted blueprint event removed - use GA_WeaponReload for cosmetics
}

void ARevolver::ApplyDamageToActor(AActor *HitActor, const FVector &HitLocation, const FVector &ShotDirection)
{
	if (!HitActor || !OwnerCharacter)
	{
		return;
	}

	// Apply damage via Unreal's damage system
	UGameplayStatics::ApplyDamage(
		HitActor,
		DamageAmount,
		OwnerCharacter->GetController(),
		this,
		UDamageType::StaticClass());
}

void ARevolver::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate weapon state and ammo (bIsEquipped is in base class)
	DOREPLIFETIME(ARevolver, CurrentAmmo);
	DOREPLIFETIME(ARevolver, bIsReloading);
}

void ARevolver::OnServerFire(const FVector &TraceStart, const FVector &TraceDir)
{
	if (!GetWorld() || !OwnerCharacter)
	{
		return;
	}

	if (!CanFire())
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnServerFire - CanFire() == false"));
		return;
	}

	// Update fire rate limiter and consume ammo
	LastFireTime = GetWorld()->GetTimeSeconds();
	int32 AmmoBefore = CurrentAmmo;
	if (CurrentAmmo > 0)
	{
		--CurrentAmmo;
	}
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnServerFire [SERVER] - %p Ammo %d->%d"), this, AmmoBefore, CurrentAmmo);
	ForceNetUpdate();

	// Apply optional spread
	FVector FireDir = TraceDir;
	if (WeaponSpread > 0.0f)
	{
		// Small random cone; convert degrees to a small vector offset approximation
		const FVector RandomSpread = FMath::VRand() * WeaponSpread;
		FireDir = (FireDir + RandomSpread).GetSafeNormal();
	}

	const FVector TraceEnd = TraceStart + (FireDir * WeaponRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_WorldDynamic,
		QueryParams);

	if (bHit && HitResult.GetActor())
	{
		ApplyDamageToActor(HitResult.GetActor(), HitResult.ImpactPoint, FireDir);

		// Trigger impact GameplayCue via owner's ASC (ensures proper replication)
		if (UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter))
		{
			FGameplayCueParameters ImpactParams;
			ImpactParams.Location = HitResult.ImpactPoint;
			ImpactParams.Normal = HitResult.ImpactNormal;
			ImpactParams.PhysicalMaterial = HitResult.PhysMaterial;
			ImpactParams.SourceObject = this;

			ASC->ExecuteGameplayCue(CattleGameplayTags::GameplayCue_Revolver_Fire_Impact, ImpactParams);
		}

		// If hit actor is a pawn, trigger hit reaction on their ASC
		if (APawn *HitPawn = Cast<APawn>(HitResult.GetActor()))
		{
			if (UAbilitySystemComponent *TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitPawn))
			{
				FGameplayCueParameters HitReactionParams;
				HitReactionParams.Location = HitResult.ImpactPoint;
				HitReactionParams.Normal = HitResult.ImpactNormal;
				HitReactionParams.Instigator = OwnerCharacter;
				HitReactionParams.SourceObject = this;

				TargetASC->ExecuteGameplayCue(CattleGameplayTags::GameplayCue_HitReaction, HitReactionParams);
			}
		}
	}
}

void ARevolver::OnRep_CurrentAmmo()
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnRep_CurrentAmmo [%s] - %p NewAmmo=%d"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), this, CurrentAmmo);
}

void ARevolver::OnRep_IsReloading()
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnRep_IsReloading [%s] - %p bIsReloading=%s"), HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), this, bIsReloading ? TEXT("TRUE") : TEXT("FALSE"));
}
