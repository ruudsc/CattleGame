#include "LassoProjectile.h"
#include "Lasso.h"
#include "LassoableComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "CattleGame/CattleGame.h"

// Frame-rate throttling for verbose logs
static int32 ProjectileTickLogCounter = 0;
static const int32 ProjectileTickLogInterval = 15; // Log every 15 frames (~0.25s at 60fps)

ALassoProjectile::ALassoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Create small collision sphere for hit detection
	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
	HitSphere->SetSphereRadius(30.0f);
	HitSphere->SetGenerateOverlapEvents(true);
	HitSphere->SetNotifyRigidBodyCollision(true);

	// Custom collision: Block world geometry (for miss), Overlap pawns (for capture without pushback)
	HitSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HitSphere->SetCollisionObjectType(ECC_WorldDynamic);
	HitSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);	 // Block ground/walls
	HitSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); // Overlap dynamic
	HitSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);		 // Overlap pawns (no push)

	RootComponent = HitSphere;

	// Create visible mesh for the flying rope loop (Visual Only)
	RopeLoopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RopeLoopMesh"));
	RopeLoopMesh->SetupAttachment(RootComponent);
	RopeLoopMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RopeLoopMesh->SetCastShadow(false);

	// Create projectile movement for arc physics
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = InitialSpeed * 1.5f;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void ALassoProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Ignore collision with owner (the player who threw the lasso)
	if (AActor *OwnerActor = GetOwner())
	{
		HitSphere->MoveIgnoreActors.Add(OwnerActor);
		UE_LOG(LogLasso, Log, TEXT("LassoProjectile::BeginPlay - Ignoring collision with owner: %s"),
			   *GetNameSafe(OwnerActor));
	}

	// Bind collision events
	HitSphere->OnComponentHit.AddDynamic(this, &ALassoProjectile::OnHit);
	HitSphere->OnComponentBeginOverlap.AddDynamic(this, &ALassoProjectile::OnOverlapBegin);

	UE_LOG(LogLasso, Log, TEXT("LassoProjectile::BeginPlay - Collision events bound, sphere radius=%.1f"),
		   HitSphere->GetScaledSphereRadius());

	// Apply configured values to movement component
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = InitialSpeed * 1.5f;
	ProjectileMovement->ProjectileGravityScale = GravityScale;

	ProjectileTickLogCounter = 0;
}

void ALassoProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasHit)
	{
		return;
	}

	// Update flight timer
	FlightTime += DeltaTime;
	if (FlightTime >= MaxFlightTime)
	{
		UE_LOG(LogLasso, Log, TEXT("LassoProjectile::Tick - Max flight time (%.1fs) reached at %s, auto-miss"),
			   MaxFlightTime, *GetActorLocation().ToString());
		OnTargetMissed();
		return;
	}

	// Throttled position/velocity logging
	ProjectileTickLogCounter++;
	if (ProjectileTickLogCounter >= ProjectileTickLogInterval)
	{
		ProjectileTickLogCounter = 0;
		UE_LOG(LogLasso, Verbose, TEXT("LassoProjectile::Tick - FlightTime=%.2fs, Pos=%s, Vel=%s (speed=%.0f)"),
			   FlightTime, *GetActorLocation().ToString(),
			   *ProjectileMovement->Velocity.ToString(), ProjectileMovement->Velocity.Size());
	}

	// Update aim assist (server-authoritative)
	if (HasAuthority())
	{
		UpdateAimAssist(DeltaTime);
	}
}

void ALassoProjectile::Launch(const FVector &Direction)
{
	FlightTime = 0.0f;
	bHasHit = false;
	AimAssistTarget = nullptr;

	// Set velocity
	ProjectileMovement->Velocity = Direction.GetSafeNormal() * InitialSpeed;

	UE_LOG(LogLasso, Log, TEXT("LassoProjectile::Launch - Launched! Speed=%.0f, Gravity=%.2f, MaxFlight=%.1fs, AimAssist(radius=%.0f, angle=%.0f)"),
		   InitialSpeed, GravityScale, MaxFlightTime, AimAssistRadius, AimAssistAngle);
	UE_LOG(LogLasso, Log, TEXT("  Direction=%s, StartPos=%s"),
		   *Direction.ToString(), *GetActorLocation().ToString());
}

void ALassoProjectile::UpdateAimAssist(float DeltaTime)
{
	// Find best target in aim assist cone
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(AimAssistRadius);

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		Sphere);

	AActor *BestTarget = nullptr;
	float BestScore = -1.0f;
	FVector CurrentDirection = ProjectileMovement->Velocity.GetSafeNormal();
	int32 ValidTargetCount = 0;

	for (const FOverlapResult &Overlap : Overlaps)
	{
		AActor *Actor = Overlap.GetActor();
		if (!IsValidTarget(Actor))
		{
			continue;
		}

		ValidTargetCount++;

		// Check if within cone angle
		FVector ToTarget = (Actor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		float DotProduct = FVector::DotProduct(CurrentDirection, ToTarget);
		float AngleRad = FMath::Acos(DotProduct);
		float AngleDeg = FMath::RadiansToDegrees(AngleRad);

		if (AngleDeg <= AimAssistAngle)
		{
			// Score based on angle (closer to center = higher score)
			float Score = DotProduct;
			if (Score > BestScore)
			{
				BestScore = Score;
				BestTarget = Actor;
			}

			// Log each valid target in cone (verbose)
			if (ProjectileTickLogCounter == 0)
			{
				UE_LOG(LogLasso, Verbose, TEXT("  AimAssist candidate: %s, angle=%.1f deg, score=%.3f"),
					   *GetNameSafe(Actor), AngleDeg, Score);
			}
		}
	}

	// Log target selection changes
	if (BestTarget != AimAssistTarget.Get())
	{
		if (BestTarget)
		{
			UE_LOG(LogLasso, Log, TEXT("LassoProjectile::UpdateAimAssist - NEW target acquired: %s (score=%.3f)"),
				   *GetNameSafe(BestTarget), BestScore);
		}
		else if (AimAssistTarget.IsValid())
		{
			UE_LOG(LogLasso, Log, TEXT("LassoProjectile::UpdateAimAssist - Lost target %s"),
				   *GetNameSafe(AimAssistTarget.Get()));
		}
	}

	// Update aim assist target
	AimAssistTarget = BestTarget;

	// Smoothly steer toward target
	if (BestTarget)
	{
		FVector ToTarget = (BestTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		FVector CurrentVelocity = ProjectileMovement->Velocity;
		float Speed = CurrentVelocity.Size();

		// Lerp direction toward target
		FVector OldDirection = CurrentVelocity.GetSafeNormal();
		FVector NewDirection = FMath::Lerp(OldDirection, ToTarget, AimAssistLerpSpeed * DeltaTime);
		ProjectileMovement->Velocity = NewDirection.GetSafeNormal() * Speed;

		// Throttled steering log
		if (ProjectileTickLogCounter == 0)
		{
			float SteerAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(OldDirection, NewDirection)));
			UE_LOG(LogLasso, Verbose, TEXT("LassoProjectile::UpdateAimAssist - Steering toward %s, steer=%.2f deg"),
				   *GetNameSafe(BestTarget), SteerAngle);
		}
	}
	else if (ProjectileTickLogCounter == 0 && Overlaps.Num() > 0)
	{
		UE_LOG(LogLasso, Verbose, TEXT("LassoProjectile::UpdateAimAssist - %d overlaps, %d valid, none in cone"),
			   Overlaps.Num(), ValidTargetCount);
	}
}

bool ALassoProjectile::IsValidTarget(AActor *Actor) const
{
	if (!Actor || Actor == GetOwner())
	{
		return false;
	}

	// Check for LassoableComponent (New RDR2 Method)
	bool bHasLassoable = Actor->FindComponentByClass<ULassoableComponent>() != nullptr;
	return bHasLassoable;
}

void ALassoProjectile::OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor,
							 UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
	UE_LOG(LogLasso, Log, TEXT("LassoProjectile::OnHit - COLLISION: Actor=%s, Component=%s, Location=%s, Normal=%s"),
		   *GetNameSafe(OtherActor), *GetNameSafe(OtherComp),
		   *Hit.ImpactPoint.ToString(), *Hit.ImpactNormal.ToString());

	if (bHasHit || !HasAuthority())
	{
		UE_LOG(LogLasso, Verbose, TEXT("  IGNORED: bHasHit=%d, HasAuthority=%d"), bHasHit, HasAuthority());
		return;
	}

	if (IsValidTarget(OtherActor))
	{
		UE_LOG(LogLasso, Log, TEXT("LassoProjectile::OnHit - VALID TARGET HIT: %s"), *GetNameSafe(OtherActor));
		OnTargetHit(OtherActor);
	}
	else
	{
		// Hit world geometry or invalid target
		UE_LOG(LogLasso, Log, TEXT("LassoProjectile::OnHit - MISS (hit non-target): %s"), *GetNameSafe(OtherActor));
		OnTargetMissed();
	}
}

void ALassoProjectile::OnOverlapBegin(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
									  UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	UE_LOG(LogLasso, Log, TEXT("LassoProjectile::OnOverlapBegin - OVERLAP: Actor=%s, Component=%s, bFromSweep=%d"),
		   *GetNameSafe(OtherActor), *GetNameSafe(OtherComp), bFromSweep);

	if (bHasHit || !HasAuthority())
	{
		UE_LOG(LogLasso, Verbose, TEXT("  IGNORED: bHasHit=%d, HasAuthority=%d"), bHasHit, HasAuthority());
		return;
	}

	if (IsValidTarget(OtherActor))
	{
		UE_LOG(LogLasso, Log, TEXT("LassoProjectile::OnOverlapBegin - VALID TARGET OVERLAP: %s"), *GetNameSafe(OtherActor));
		OnTargetHit(OtherActor);
	}
	else
	{
		UE_LOG(LogLasso, Verbose, TEXT("  Target %s not valid (no LassoableComponent)"), *GetNameSafe(OtherActor));
	}
}

void ALassoProjectile::OnTargetHit(AActor *Target)
{
	UE_LOG(LogLasso, Log, TEXT("LassoProjectile::OnTargetHit - Processing hit on %s at %s, FlightTime=%.2fs"),
		   *GetNameSafe(Target), *GetActorLocation().ToString(), FlightTime);

	bHasHit = true;
	ProjectileMovement->StopMovementImmediately();

	if (LassoWeapon)
	{
		LassoWeapon->OnProjectileHitTarget(Target);
	}
	else
	{
		UE_LOG(LogLasso, Error, TEXT("LassoProjectile::OnTargetHit - No LassoWeapon reference!"));
	}
}

void ALassoProjectile::OnTargetMissed()
{
	UE_LOG(LogLasso, Log, TEXT("LassoProjectile::OnTargetMissed - Projectile missed at %s, FlightTime=%.2fs"),
		   *GetActorLocation().ToString(), FlightTime);

	bHasHit = true;
	ProjectileMovement->StopMovementImmediately();

	if (LassoWeapon)
	{
		LassoWeapon->OnProjectileMissed();
	}
	else
	{
		UE_LOG(LogLasso, Error, TEXT("LassoProjectile::OnTargetMissed - No LassoWeapon reference!"));
	}
}

void ALassoProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
