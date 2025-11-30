#include "WeaponPickup.h"
#include "WeaponBase.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

#include "CattleGame/CattleGame.h"

AWeaponPickup::AWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.01f;
	bReplicates = true;

	// Create root sphere component for collision
	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetSphereRadius(50.0f);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = TriggerSphere;

	// Create visual mesh component
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->AttachToComponent(TriggerSphere, FAttachmentTransformRules::SnapToTargetIncludingScale);
}

void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();

	// Store original location for bobbing animation
	OriginalLocation = GetActorLocation();

	// Bind overlap event
	if (TriggerSphere)
	{
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnSphereBeginOverlap);
	}

	// Spawn weapon if configured to do so
	if (bSpawnWeaponOnBeginPlay && WeaponClass)
	{
		SpawnWeapon();
	}
}

void AWeaponPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update animation time
	AnimationTime += DeltaTime;

	// Rotate pickup mesh
	if (PickupMesh)
	{
		FRotator CurrentRotation = PickupMesh->GetRelativeRotation();
		CurrentRotation.Yaw += RotationSpeed * DeltaTime;
		PickupMesh->SetRelativeRotation(CurrentRotation);

		// Bob up and down
		FVector MeshPosition = PickupMesh->GetRelativeLocation();
		MeshPosition.Z = FMath::Sin(AnimationTime * BobSpeed) * BobHeight;
		PickupMesh->SetRelativeLocation(MeshPosition);
	}
}

void AWeaponPickup::SpawnWeapon()
{
	if (!WeaponClass)
	{
		UE_LOG(LogGASDebug, Error, TEXT("WeaponPickup: WeaponClass is not set!"));
		return;
	}

	if (SpawnedWeapon)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("WeaponPickup: Weapon already spawned!"));
		return;
	}

	UE_LOG(LogGASDebug, Log, TEXT("WeaponPickup: Spawning weapon..."));

	// Spawn weapon at pickup location
	SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass);
	if (SpawnedWeapon)
	{
		UE_LOG(LogGASDebug, Log, TEXT("WeaponPickup: Weapon spawned successfully!"));
		// Position it at pickup location but don't attach
		SpawnedWeapon->SetActorLocation(GetActorLocation());
		SpawnedWeapon->SetActorRotation(FRotator::ZeroRotator);
		SpawnedWeapon->SetOwner(this);
	}
	else
	{
		UE_LOG(LogGASDebug, Error, TEXT("WeaponPickup: Failed to spawn weapon actor!"));
	}
}

void AWeaponPickup::OnSphereBeginOverlap(
	UPrimitiveComponent *OverlappedComponent,
	AActor *OtherActor,
	UPrimitiveComponent *OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult &SweepResult)
{

	// Only server processes pickups
	if (!HasAuthority())
	{
		UE_LOG(LogGASDebug, Warning, TEXT("WeaponPickup: Not authority, ignoring pickup"));
		return;
	}

	// Check if overlapping actor is a character
	ACattleCharacter *Character = Cast<ACattleCharacter>(OtherActor);
	if (Character)
	{
		UE_LOG(LogGASDebug, Log, TEXT("WeaponPickup: Detected CattleCharacter!"));

		// Spawn weapon if not already spawned
		if (!SpawnedWeapon && WeaponClass)
		{
			SpawnWeapon();
		}

		// Try to give weapon to character
		if (SpawnedWeapon)
		{
			AttemptPickup(Character);
		}
		else
		{
			UE_LOG(LogGASDebug, Error, TEXT("WeaponPickup: Failed to spawn weapon!"));
		}
	}
}

void AWeaponPickup::AttemptPickup(ACattleCharacter *Character)
{
	if (!Character || !SpawnedWeapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("WeaponPickup: Character or SpawnedWeapon is null!"));
		return;
	}

	// Get character's inventory
	UInventoryComponent *Inventory = Character->GetInventoryComponent();
	if (!Inventory)
	{
		UE_LOG(LogGASDebug, Error, TEXT("WeaponPickup: Character has no InventoryComponent!"));
		return;
	}

	UE_LOG(LogGASDebug, Log, TEXT("WeaponPickup: Attempting to add weapon to inventory..."));

	// Try to add weapon to first available pickup slot
	int32 SlotIndex = Inventory->AddWeaponToFirstAvailableSlot(SpawnedWeapon);

	if (SlotIndex >= 0)
	{
		// Successfully picked up
		UE_LOG(LogGASDebug, Log, TEXT("WeaponPickup: SUCCESS! Weapon added to slot %d"), SlotIndex);
		OnWeaponPickedUp.Broadcast(Character);
		DestroyPickup();
	}
	else
	{
		UE_LOG(LogGASDebug, Error, TEXT("WeaponPickup: No available slots in inventory!"));
	}
	// If no space, weapon stays in world and can be picked up later
}

void AWeaponPickup::DestroyPickup()
{
	Destroy();
}
