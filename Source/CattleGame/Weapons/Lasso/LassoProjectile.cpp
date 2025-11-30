#include "LassoProjectile.h"
#include "Lasso.h"
#include "Components/SphereComponent.h"
#include "CableComponent/Classes/CableComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "CattleGame/CattleGame.h"
#include "Kismet/KismetSystemLibrary.h"

ALassoProjectile::ALassoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// Create collision sphere as root
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(25.0f); // Larger for aim assist
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // Overlap with pawns for aim assist
	RootComponent = CollisionSphere;

	// Create mesh component for rope loop visual
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionSphere);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create cable component for rope visual
	CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
	CableComponent->SetupAttachment(CollisionSphere);
	CableComponent->CableLength = 0.0f; // Will be updated dynamically
	CableComponent->CableWidth = 2.0f;
	CableComponent->NumSegments = 10;
	CableComponent->bEnableStiffness = false;
	CableComponent->SolverIterations = 4;

	// Create projectile movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 1500.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->Friction = 0.0f;
	ProjectileMovement->ProjectileGravityScale = 0.5f; // Arc trajectory
}

void ALassoProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Bind collision and overlap events
	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ALassoProjectile::OnCollision);
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALassoProjectile::OnOverlapBegin);
	}

	// Store initial owner location for range checking
	if (GetOwner())
	{
		OwnerInitialLocation = GetOwner()->GetActorLocation();

		// Attach cable end to owner's hand socket
		if (CableComponent)
		{
			if (ACharacter *OwnerCharacter = Cast<ACharacter>(GetOwner()))
			{
				CableComponent->SetAttachEndTo(OwnerCharacter, NAME_None, FName("hand_rSocket"));
			}
		}
	}

	UE_LOG(LogGASDebug, Log, TEXT("LassoProjectile::BeginPlay - Lasso projectile spawned"));
}

void ALassoProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle retract mode
	if (bIsRetracting)
	{
		TickRetract(DeltaTime);
		return;
	}

	// Update distance traveled
	if (GetOwner())
	{
		float CurrentDistance = FVector::Dist(GetActorLocation(), OwnerInitialLocation);
		DistanceTraveled = CurrentDistance;

		// Check if lasso has exceeded range (auto-retract)
		if (DistanceTraveled > LassoRange)
		{
			UE_LOG(LogGASDebug, Log, TEXT("LassoProjectile::Tick - Exceeded range, auto-retracting"));

			// Notify weapon of miss
			if (LassoWeapon && HasAuthority())
			{
				LassoWeapon->OnProjectileMissed();
			}

			StartRetract();
			return;
		}
	}

	// Update aim assist
	if (HasAuthority())
	{
		UpdateAimAssist();
	}
}

void ALassoProjectile::Launch(const FVector &Direction, float Speed)
{
	if (!ProjectileMovement)
	{
		return;
	}

	// Reset state
	bIsRetracting = false;
	bHasCaughtTarget = false;
	AimAssistTarget.Reset();
	DistanceTraveled = 0.0f;

	// Re-enable collision (in case it was disabled from previous catch)
	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// Store initial location
	if (GetOwner())
	{
		OwnerInitialLocation = GetOwner()->GetActorLocation();
	}

	FVector NormalizedDirection = Direction.GetSafeNormal();
	ProjectileMovement->Velocity = NormalizedDirection * Speed;
	ProjectileMovement->Activate(true);

	UE_LOG(LogGASDebug, Warning, TEXT("LassoProjectile::Launch - Launched with velocity %s, gravity=%.2f"),
		   *ProjectileMovement->Velocity.ToString(), ProjectileMovement->ProjectileGravityScale);
}

void ALassoProjectile::SetLassoProperties(float Speed, float MaxRange, float Gravity, float AimAssist, float AimAssistRange, float Retract)
{
	LassoSpeed = Speed;
	LassoRange = MaxRange;
	GravityScale = Gravity;
	AimAssistAngle = AimAssist;
	AimAssistMaxDistance = AimAssistRange;
	RetractSpeed = Retract;

	// Apply gravity scale
	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = GravityScale;
	}
}

void ALassoProjectile::SetLassoWeapon(ALasso *Weapon)
{
	LassoWeapon = Weapon;
}

void ALassoProjectile::StartRetract()
{
	if (bIsRetracting)
	{
		return;
	}

	bIsRetracting = true;

	// Stop projectile movement
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	UE_LOG(LogGASDebug, Warning, TEXT("LassoProjectile::StartRetract - Beginning retraction at %.0f cm/s"), RetractSpeed);
}

void ALassoProjectile::TickRetract(float DeltaTime)
{
	if (!GetOwner())
	{
		UE_LOG(LogGASDebug, Warning, TEXT("LassoProjectile::TickRetract - No owner!"));
		return;
	}

	// Move toward owner
	FVector OwnerLocation = GetOwner()->GetActorLocation();
	FVector CurrentLocation = GetActorLocation();
	FVector Direction = (OwnerLocation - CurrentLocation).GetSafeNormal();

	float MoveDistance = RetractSpeed * DeltaTime;
	float RemainingDistance = FVector::Dist(CurrentLocation, OwnerLocation);

	if (MoveDistance >= RemainingDistance)
	{
		// We've reached the owner
		SetActorLocation(OwnerLocation);
		UE_LOG(LogGASDebug, Warning, TEXT("LassoProjectile::TickRetract - Reached owner, calling OnRetractComplete"));

		// Notify weapon we're done
		if (LassoWeapon && HasAuthority())
		{
			LassoWeapon->OnRetractComplete();
		}
	}
	else
	{
		// Move toward owner
		SetActorLocation(CurrentLocation + Direction * MoveDistance);
	}
}

void ALassoProjectile::UpdateAimAssist()
{
	if (!GetOwner() || bIsRetracting)
	{
		return;
	}

	// Get current direction of travel
	FVector Velocity = ProjectileMovement ? ProjectileMovement->Velocity : FVector::ZeroVector;
	if (Velocity.IsNearlyZero())
	{
		return;
	}

	FVector CurrentDirection = Velocity.GetSafeNormal();
	FVector CurrentLocation = GetActorLocation();

	// Sphere cast for targets in aim assist cone
	TArray<FHitResult> OutHits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	// Look for targets in a sphere ahead
	FVector SphereCenter = CurrentLocation + CurrentDirection * (AimAssistMaxDistance * 0.5f);
	float SphereRadius = FMath::Tan(FMath::DegreesToRadians(AimAssistAngle)) * AimAssistMaxDistance;

	bool bHit = GetWorld()->SweepMultiByChannel(
		OutHits,
		CurrentLocation,
		CurrentLocation + CurrentDirection * AimAssistMaxDistance,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(SphereRadius),
		QueryParams);

	if (bHit && OutHits.Num() > 0)
	{
		// Find closest valid target
		AActor *ClosestTarget = nullptr;
		float ClosestDist = FLT_MAX;

		for (const FHitResult &Hit : OutHits)
		{
			AActor *HitActor = Hit.GetActor();
			if (!HitActor || !HitActor->IsA<APawn>())
			{
				continue;
			}

			// Check angle from current trajectory
			FVector ToTarget = (HitActor->GetActorLocation() - CurrentLocation).GetSafeNormal();
			float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(CurrentDirection, ToTarget)));

			if (Angle <= AimAssistAngle)
			{
				float Dist = FVector::Dist(CurrentLocation, HitActor->GetActorLocation());
				if (Dist < ClosestDist)
				{
					ClosestDist = Dist;
					ClosestTarget = HitActor;
				}
			}
		}

		if (ClosestTarget && ClosestTarget != AimAssistTarget.Get())
		{
			AimAssistTarget = ClosestTarget;

			// Gently adjust trajectory toward target
			FVector ToTarget = (ClosestTarget->GetActorLocation() - CurrentLocation).GetSafeNormal();
			FVector NewVelocity = FMath::Lerp(CurrentDirection, ToTarget, 0.1f).GetSafeNormal() * Velocity.Size();

			if (ProjectileMovement)
			{
				ProjectileMovement->Velocity = NewVelocity;
			}

			UE_LOG(LogGASDebug, Log, TEXT("LassoProjectile::UpdateAimAssist - Assisting toward %s"), *ClosestTarget->GetName());
		}
	}
}

void ALassoProjectile::OnCollision(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
								   FVector NormalImpulse, const FHitResult &Hit)
{
	// Only process on server
	if (!HasAuthority())
	{
		return;
	}

	// Don't hit owner
	if (OtherActor == GetOwner())
	{
		return;
	}

	// Don't process while retracting
	if (bIsRetracting)
	{
		return;
	}

	// Hit environment - start retracting
	UE_LOG(LogGASDebug, Warning, TEXT("LassoProjectile::OnCollision - Hit environment %s"), *GetNameSafe(OtherActor));

	// Notify weapon of miss
	if (LassoWeapon)
	{
		LassoWeapon->OnProjectileMissed();
	}

	StartRetract();
}

void ALassoProjectile::OnOverlapBegin(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
									  int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	// Only process on server
	if (!HasAuthority())
	{
		return;
	}

	// Don't hit owner
	if (OtherActor == GetOwner())
	{
		return;
	}

	// Don't process while retracting or already caught
	if (bIsRetracting || bHasCaughtTarget)
	{
		return;
	}

	// Only catch pawns
	if (!OtherActor->IsA<APawn>())
	{
		return;
	}

	// Mark as caught to prevent further overlaps
	bHasCaughtTarget = true;

	// Stop movement
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	// Disable further collision/overlap events
	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Notify weapon of hit
	if (LassoWeapon)
	{
		LassoWeapon->OnProjectileHitTarget(OtherActor);
	}

	UE_LOG(LogGASDebug, Warning, TEXT("LassoProjectile::OnOverlapBegin - Caught %s"), *OtherActor->GetName());
}

void ALassoProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALassoProjectile, bIsRetracting);
}
