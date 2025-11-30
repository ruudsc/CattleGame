#include "Revolver.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
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

void ARevolver::EquipWeapon()
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::EquipWeapon() - Addr: %p, HasAuthority: %s, OwnerCharacter: %p"),
		   this, HasAuthority() ? TEXT("TRUE") : TEXT("FALSE"), OwnerCharacter);

	// Call the base implementation which fires the event
	Super::EquipWeapon();

	// Attach to character hand socket
	AttachToCharacterHand();

	// Also explicitly call our C++ implementation to ensure bIsEquipped is set
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::EquipWeapon() - CurrentOwner resolved to: %p"), CurrentOwner);
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
	OnWeaponEquipped_Implementation(CurrentOwner, ActiveMesh);
}

void ARevolver::UnequipWeapon()
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::UnequipWeapon() - Addr: %p"), this);

	// Detach from character
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Call unequip implementation
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
	OnWeaponUnequipped_Implementation(CurrentOwner, ActiveMesh);

	// Call base to fire event
	Super::UnequipWeapon();
}

void ARevolver::Fire()
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::Fire() - Addr: %p, HasAuthority: %s"),
		   this, HasAuthority() ? TEXT("TRUE") : TEXT("FALSE"));

	// Trigger blueprint event for local cosmetics
	Super::Fire();

	// Client-predicted fire: compute trace start/dir and request server fire
	if (!CanFire())
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Revolver::Fire() - CanFire() == false"));
		return;
	}

	FVector TraceStart, TraceDir;
	GetTraceStartAndDirection(TraceStart, TraceDir);
	RequestServerFireWithPrediction(TraceStart, TraceDir);
}

void ARevolver::OnWeaponEquipped_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh)
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnWeaponEquipped BEFORE - Addr: %p, HasAuthority: %s, Equipped: %s"),
		   this, HasAuthority() ? TEXT("TRUE") : TEXT("FALSE"), bIsEquipped ? TEXT("TRUE") : TEXT("FALSE"));

	bIsEquipped = true;

	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnWeaponEquipped SET - Addr: %p, HasAuthority: %s, Equipped: %s"),
		   this, HasAuthority() ? TEXT("TRUE") : TEXT("FALSE"), bIsEquipped ? TEXT("TRUE") : TEXT("FALSE"));
	if (Character)
	{
		OwnerCharacter = Character;
	}
	(void)Mesh;
	// Blueprints can listen to this event to handle mesh visibility and animations
}

void ARevolver::OnWeaponUnequipped_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh)
{
	UE_LOG(LogGASDebug, Warning, TEXT("Revolver::OnWeaponUnequipped - Addr: %p, Setting Equipped = FALSE"), this);

	bIsEquipped = false;
	bIsReloading = false;
	(void)Character;
	(void)Mesh;

	// Clear reload timer if active
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	// Blueprints can listen to this event to handle mesh hiding and cleanup
}

void ARevolver::OnWeaponFired_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh)
{
	// Keep only minimal bookkeeping for cosmetic hooks; authority logic handled in OnServerFire
	if (Character)
	{
		OwnerCharacter = Character;
	}
	(void)Mesh;
	UE_LOG(LogGASDebug, Verbose, TEXT("Revolver::OnWeaponFired_Implementation - cosmetic only (server handles firing)"));
}

void ARevolver::OnReloadStarted_Implementation(ACattleCharacter *Character, USkeletalMeshComponent *Mesh)
{
	if (Character)
	{
		OwnerCharacter = Character;
	}
	(void)Mesh;
	if (!CanReload())
	{
		return;
	}

	bIsReloading = true;

	// Set timer for reload completion
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&ARevolver::OnReloadComplete,
			ReloadTime,
			false);
	}

	// Blueprints can listen to this event to handle reload animations and VFX
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

	// Notify blueprints that reload is complete
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
	OnReloadCompleted(CurrentOwner, ActiveMesh);
}

void ARevolver::PerformLineTrace()
{
	if (!OwnerCharacter || !GetWorld())
	{
		return;
	}

	// Get trace start and direction from camera
	FVector TraceStart;
	FVector TraceDirection;
	GetTraceStartAndDirection(TraceStart, TraceDirection);

	// Apply spread to direction (random cone)
	if (WeaponSpread > 0.0f)
	{
		FVector RandomSpread = FMath::VRand() * WeaponSpread;
		TraceDirection = (TraceDirection + RandomSpread).GetSafeNormal();
	}

	FVector TraceEnd = TraceStart + (TraceDirection * WeaponRange);

	// Perform line trace
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_WorldDynamic,
		QueryParams);

	if (bHit && HitResult.GetActor())
	{
		// Apply damage
		ApplyDamageToActor(HitResult.GetActor(), HitResult.ImpactPoint, TraceDirection);

		// Play impact effect
		PlayImpactEffect(HitResult.ImpactPoint, HitResult.ImpactNormal);

		// Debug draw (remove in production)
		// DrawDebugLine(GetWorld(), TraceStart, HitResult.ImpactPoint, FColor::Red, false, 1.0f);
	}
	else
	{
		// No hit - draw to max range
		// DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 1.0f);
	}
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

	// Could add hit reactions, knockback, etc. here in the future
	if (bApplyHitReaction)
	{
		// This could trigger hit reactions on the target character
		// For now, just apply damage
	}
}

void ARevolver::PlayMuzzleFlash()
{
	// VFX should be handled in blueprints now
	// Blueprints can listen to OnWeaponFired event
}

void ARevolver::PlayImpactEffect(const FVector &ImpactLocation, const FVector &ImpactNormal)
{
	if (!ImpactEffect)
	{
		return;
	}

	// Spawn impact effect at hit location, oriented along surface normal
	FRotator ImpactRotation = ImpactNormal.Rotation();

	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		ImpactEffect,
		ImpactLocation,
		ImpactRotation);
}

void ARevolver::PlayFireSound()
{
	if (!FireSound || !OwnerCharacter)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		GetWorld(),
		FireSound,
		OwnerCharacter->GetActorLocation());
}

void ARevolver::GetTraceStartAndDirection(FVector &OutStart, FVector &OutDirection) const
{
	if (!OwnerCharacter)
	{
		OutStart = FVector::ZeroVector;
		OutDirection = FVector::ForwardVector;
		return;
	}

	// Get camera location and forward direction
	FVector CameraLocation = OwnerCharacter->GetFirstPersonCameraComponent()->GetComponentLocation();
	FVector CameraDirection = OwnerCharacter->GetFirstPersonCameraComponent()->GetForwardVector();

	OutStart = CameraLocation;
	OutDirection = CameraDirection;
}

void ARevolver::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate weapon state and ammo
	DOREPLIFETIME(ARevolver, CurrentAmmo);
	DOREPLIFETIME(ARevolver, bIsEquipped);
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
		PlayImpactEffect(HitResult.ImpactPoint, HitResult.ImpactNormal);
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

void ARevolver::AttachToCharacterHand()
{
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	if (!CurrentOwner)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Revolver::AttachToCharacterHand - No owner character"));
		return;
	}

	USkeletalMeshComponent *TargetMesh = CurrentOwner->GetActiveCharacterMesh();
	if (!TargetMesh)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Revolver::AttachToCharacterHand - No active character mesh"));
		return;
	}

	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
	AttachToComponent(TargetMesh, AttachRules, RevolverHandSocketName);

	UE_LOG(LogGASDebug, Log, TEXT("Revolver::AttachToCharacterHand - Attached to socket %s on mesh %s"), *RevolverHandSocketName.ToString(), *TargetMesh->GetName());
}
