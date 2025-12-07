#include "Lasso.h"
#include "LassoProjectile.h"
#include "LassoableComponent.h"
#include "LassoLoopActor.h"
#include "CableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "Net/UnrealNetwork.h"
#include "CattleGame/CattleGame.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

// Frame-rate throttling for verbose logs
static int32 LassoTickLogCounter = 0;
static const int32 LassoTickLogInterval = 30; // Log every 30 frames (~0.5s at 60fps)

ALasso::ALasso()
{
	WeaponSlotID = 1; // Lasso slot
	WeaponName = FString(TEXT("Lasso"));
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	// Create root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Create lasso coil mesh in hand (visible during throw)
	HandCoilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandCoilMesh"));
	HandCoilMesh->SetupAttachment(RootComponent);
	HandCoilMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create cable component for rope visual (purely cosmetic, no physics)
	RopeCable = CreateDefaultSubobject<UCableComponent>(TEXT("RopeCable"));
	RopeCable->SetupAttachment(RootComponent);
	RopeCable->bAttachStart = true;
	RopeCable->bAttachEnd = false;
	RopeCable->CableLength = 100.0f;
	RopeCable->NumSegments = 10;
	RopeCable->CableWidth = 3.0f;
	RopeCable->bEnableStiffness = false;
	RopeCable->SetEnableGravity(false); // No gravity sag simulation
	RopeCable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RopeCable->SetVisibility(false); // Hidden until tethered
}

void ALasso::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickCooldown(DeltaTime);

	if (HasAuthority())
	{
		switch (CurrentState)
		{
		case ELassoState::Throwing:
			TickThrowing(DeltaTime);
			break;
		case ELassoState::Tethered:
			TickTethered(DeltaTime);
			break;
		case ELassoState::Retracting:
			TickRetracting(DeltaTime);
			break;
		default:
			break;
		}
	}

	// Update cable visual on all instances
	UpdateCableVisual();
}

// ===== WEAPON INTERFACE =====

bool ALasso::CanFire() const
{
	bool bCanFire = CurrentState == ELassoState::Idle && CooldownRemaining <= 0.0f;
	if (!bCanFire)
	{
		const TCHAR *StateNames[] = {TEXT("Idle"), TEXT("Throwing"), TEXT("Tethered"), TEXT("Retracting")};
		UE_LOG(LogLasso, Log, TEXT("Lasso::CanFire - BLOCKED: State=%s (need Idle), Cooldown=%.2f"),
			   StateNames[(int32)CurrentState], CooldownRemaining);
	}
	return bCanFire;
}

void ALasso::StartPulling()
{
	if (CurrentState == ELassoState::Tethered && !bIsPulling)
	{
		bIsPulling = true;
		UE_LOG(LogLasso, Warning, TEXT("Lasso::StartPulling - Started pulling target %s, ConstraintLength=%.1f"),
			   *GetNameSafe(TetheredTarget), ConstraintLength);
	}
	else
	{
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::StartPulling - IGNORED: State=%d, bIsPulling=%d"),
			   (int32)CurrentState, bIsPulling);
	}
}

void ALasso::StopPulling()
{
	if (bIsPulling)
	{
		bIsPulling = false;
		UE_LOG(LogLasso, Warning, TEXT("Lasso::StopPulling - Stopped pulling, final ConstraintLength=%.1f"),
			   ConstraintLength);
	}
}

void ALasso::ReleaseTether()
{
	if (CurrentState != ELassoState::Tethered)
	{
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::ReleaseTether - IGNORED: Not tethered (State=%d)"),
			   (int32)CurrentState);
		return;
	}

	if (!HasAuthority())
	{
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::ReleaseTether - Client calling ServerReleaseTether"));
		ServerReleaseTether();
		return;
	}

	UE_LOG(LogLasso, Warning, TEXT("Lasso::ReleaseTether - Releasing target %s"), *GetNameSafe(TetheredTarget));

	// Notify target before clearing
	if (TetheredTarget)
	{
		if (ULassoableComponent *Lassoable = TetheredTarget->FindComponentByClass<ULassoableComponent>())
		{
			Lassoable->OnReleased();
		}
	}

	// Destroy loop mesh
	DestroyLoopMesh();

	TetheredTarget = nullptr;
	bIsPulling = false;

	OnTargetReleased();

	SetState(ELassoState::Retracting);
}

void ALasso::ForceReset()
{
	UE_LOG(LogLasso, Warning, TEXT("Lasso::ForceReset - Hard reset from State=%d, Target=%s"),
		   (int32)CurrentState, *GetNameSafe(TetheredTarget));

	DestroyProjectile();
	DestroyLoopMesh();
	TetheredTarget = nullptr;
	bIsPulling = false;
	CooldownRemaining = 0.0f;
	RetractTimer = 0.0f;

	RopeCable->SetVisibility(false);
	HandCoilMesh->SetVisibility(true);

	SetState(ELassoState::Idle);
}

// ===== PROJECTILE CALLBACKS =====

void ALasso::OnProjectileHitTarget(AActor *Target)
{
	if (!HasAuthority() || CurrentState != ELassoState::Throwing)
	{
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::OnProjectileHitTarget - IGNORED: HasAuthority=%d, State=%d"),
			   HasAuthority(), (int32)CurrentState);
		return;
	}

	UE_LOG(LogLasso, Warning, TEXT("Lasso::OnProjectileHitTarget - CAPTURED target %s at location %s"),
		   *GetNameSafe(Target), *Target->GetActorLocation().ToString());

	TetheredTarget = Target;

	// Set constraint length to current distance
	if (OwnerCharacter && Target)
	{
		ConstraintLength = FVector::Dist(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
		ConstraintLength = FMath::Min(ConstraintLength, MaxConstraintLength);
	}

	// Spawn loop mesh on target
	SpawnLoopMeshOnTarget(Target);

	// Show cable
	RopeCable->SetVisibility(true);

	// Notify target's lassoable component
	if (ULassoableComponent *Lassoable = Target->FindComponentByClass<ULassoableComponent>())
	{
		Lassoable->OnCaptured(OwnerCharacter);
	}

	OnTargetCaptured(Target);

	// Destroy the projectile now that we're tethered (visuals handled by rope/loop)
	DestroyProjectile();

	SetState(ELassoState::Tethered);
}

void ALasso::OnProjectileMissed()
{
	if (!HasAuthority() || CurrentState != ELassoState::Throwing)
	{
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::OnProjectileMissed - IGNORED: HasAuthority=%d, State=%d"),
			   HasAuthority(), (int32)CurrentState);
		return;
	}

	UE_LOG(LogLasso, Log, TEXT("Lasso::OnProjectileMissed - Projectile missed, entering retract"));

	SetState(ELassoState::Retracting);
}

// ===== BLUEPRINT EVENTS =====

void ALasso::OnLassoThrown_Implementation()
{
	UE_LOG(LogLasso, Log, TEXT("Lasso::OnLassoThrown - Projectile launched"));
}

void ALasso::OnTargetCaptured_Implementation(AActor *Target)
{
	UE_LOG(LogLasso, Warning, TEXT("Lasso::OnTargetCaptured - Target=%s, ConstraintLength=%.1f"),
		   *GetNameSafe(Target), ConstraintLength);
}

void ALasso::OnTargetReleased_Implementation()
{
	UE_LOG(LogLasso, Log, TEXT("Lasso::OnTargetReleased - Tether released"));
}

void ALasso::OnRetractComplete_Implementation()
{
	// Show hand coil mesh again, hide cable
	HandCoilMesh->SetVisibility(true);
	RopeCable->SetVisibility(false);
	UE_LOG(LogLasso, Log, TEXT("Lasso::OnRetractComplete - Ready for next throw"));
}

// ===== SERVER RPCS =====

void ALasso::ServerFire_Implementation(const FVector_NetQuantize &SpawnLocation, const FVector_NetQuantizeNormal &LaunchDirection)
{
	if (!CanFire())
	{
		UE_LOG(LogLasso, Warning, TEXT("Lasso::ServerFire - REJECTED: CanFire=false"));
		return;
	}

	UE_LOG(LogLasso, Warning, TEXT("Lasso::ServerFire - Spawning projectile at %s, direction %s"),
		   *FVector(SpawnLocation).ToString(), *FVector(LaunchDirection).ToString());

	SpawnProjectile(SpawnLocation, LaunchDirection);
	SetState(ELassoState::Throwing);
	OnLassoThrown();
}

void ALasso::ServerReleaseTether_Implementation()
{
	ReleaseTether();
}

// ===== NETWORK =====

void ALasso::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALasso, CurrentState);
	DOREPLIFETIME(ALasso, TetheredTarget);
}

void ALasso::OnRep_CurrentState()
{
	const TCHAR *StateNames[] = {TEXT("Idle"), TEXT("Throwing"), TEXT("Tethered"), TEXT("Retracting")};
	UE_LOG(LogLasso, Log, TEXT("Lasso::OnRep_CurrentState - Replicated state: %s (%d)"),
		   StateNames[(int32)CurrentState], (int32)CurrentState);

	// Update visuals on clients
	switch (CurrentState)
	{
	case ELassoState::Idle:
		HandCoilMesh->SetVisibility(true);
		RopeCable->SetVisibility(false);
		break;
	case ELassoState::Throwing:
		// HandCoilMesh stays visible
		// Rope is visible and connects to projectile
		RopeCable->SetVisibility(true);
		break;
	case ELassoState::Tethered:
		RopeCable->SetVisibility(true);
		break;
	case ELassoState::Retracting:
		break;
	}
}

// ===== INTERNAL =====

void ALasso::SetState(ELassoState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	ELassoState OldState = CurrentState;
	CurrentState = NewState;

	const TCHAR *StateNames[] = {TEXT("Idle"), TEXT("Throwing"), TEXT("Tethered"), TEXT("Retracting")};
	UE_LOG(LogLasso, Warning, TEXT("Lasso::SetState - Transition: %s -> %s"),
		   StateNames[(int32)OldState], StateNames[(int32)NewState]);

	// State enter logic
	if (NewState == ELassoState::Retracting)
	{
		RetractTimer = 0.0f;
		TetheredTarget = nullptr;
		bIsPulling = false;
	}
}

void ALasso::TickThrowing(float DeltaTime)
{
	// Projectile handles everything, just wait for callbacks
}

void ALasso::TickTethered(float DeltaTime)
{
	if (!TetheredTarget || !OwnerCharacter)
	{
		UE_LOG(LogLasso, Warning, TEXT("Lasso::TickTethered - Lost target or owner, releasing (Target=%s, Owner=%s)"),
			   *GetNameSafe(TetheredTarget), *GetNameSafe(OwnerCharacter));
		SetState(ELassoState::Retracting);
		return;
	}

	// Apply constraint if pulling
	if (bIsPulling)
	{
		ApplyConstraintForce(DeltaTime);

		// Slowly shorten constraint (reel in)
		float OldConstraint = ConstraintLength;
		ConstraintLength = FMath::Max(100.0f, ConstraintLength - PullReelSpeed * DeltaTime);

		// Throttled verbose logging
		LassoTickLogCounter++;
		if (LassoTickLogCounter >= LassoTickLogInterval)
		{
			LassoTickLogCounter = 0;
			UE_LOG(LogLasso, Verbose, TEXT("Lasso::TickTethered - Reeling: Constraint %.1f -> %.1f, ReelSpeed=%.1f"),
				   OldConstraint, ConstraintLength, PullReelSpeed);
		}
	}
}

void ALasso::TickRetracting(float DeltaTime)
{
	RetractTimer += DeltaTime;

	if (RetractTimer >= RetractDuration)
	{
		DestroyProjectile();
		CooldownRemaining = ThrowCooldown;
		UE_LOG(LogLasso, Log, TEXT("Lasso::TickRetracting - Retract complete, starting cooldown: %.2f seconds"), ThrowCooldown);
		OnRetractComplete();
		SetState(ELassoState::Idle);
	}
}

void ALasso::TickCooldown(float DeltaTime)
{
	if (CooldownRemaining > 0.0f)
	{
		float OldCooldown = CooldownRemaining;
		CooldownRemaining -= DeltaTime;
		if (CooldownRemaining <= 0.0f)
		{
			CooldownRemaining = 0.0f;
			UE_LOG(LogLasso, Log, TEXT("Lasso::TickCooldown - Cooldown finished, ready to throw!"));
		}
	}
}

void ALasso::ApplyConstraintForce(float DeltaTime)
{
	if (!TetheredTarget || !OwnerCharacter)
	{
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::ApplyConstraintForce - SKIPPED: Missing target or owner"));
		return;
	}

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = TetheredTarget->GetActorLocation();
	float Distance = FVector::Dist(OwnerLocation, TargetLocation);

	// Only apply force if stretched beyond constraint length
	if (Distance <= ConstraintLength)
	{
		// Throttled verbose log for slack rope
		if (LassoTickLogCounter == 0)
		{
			UE_LOG(LogLasso, Verbose, TEXT("Lasso::ApplyConstraintForce - SLACK: Distance=%.1f <= Constraint=%.1f (no force)"),
				   Distance, ConstraintLength);
		}
		return;
	}

	FVector Direction = (TargetLocation - OwnerLocation).GetSafeNormal();
	float Overshoot = Distance - ConstraintLength;

	// Get masses for force distribution
	float OwnerMass = 100.0f;
	float TargetMass = 100.0f;

	UCharacterMovementComponent *OwnerMovement = OwnerCharacter->GetCharacterMovement();
	UCharacterMovementComponent *TargetMovement = nullptr;

	if (OwnerMovement)
	{
		OwnerMass = OwnerMovement->Mass;
	}

	ACharacter *TargetChar = Cast<ACharacter>(TetheredTarget);
	if (TargetChar)
	{
		TargetMovement = TargetChar->GetCharacterMovement();
		if (TargetMovement)
		{
			TargetMass = TargetMovement->Mass;
		}
	}

	float TotalMass = OwnerMass + TargetMass;
	float OwnerRatio = TargetMass / TotalMass;
	float TargetRatio = OwnerMass / TotalMass;

	// Force scales with overshoot for elastic feel, but cap it
	float ForceMultiplier = FMath::Min(Overshoot / 100.0f, 3.0f); // Cap at 3x

	// Calculate velocity change based on force (F = ma, so a = F/m, and dv = a*dt)
	// For characters, we apply this as a velocity impulse each frame
	float AccelerationMagnitude = (PullForce * ForceMultiplier) / 100.0f; // Normalize by base mass
	FVector VelocityChange = Direction * AccelerationMagnitude * DeltaTime;

	// Throttled detailed force logging
	if (LassoTickLogCounter == 0)
	{
		UE_LOG(LogLasso, Log, TEXT("Lasso::ApplyConstraintForce - TAUT: Distance=%.1f, Constraint=%.1f, Overshoot=%.1f"),
			   Distance, ConstraintLength, Overshoot);
		UE_LOG(LogLasso, Log, TEXT("  Mass: Owner=%.1f, Target=%.1f, Ratios=(%.2f, %.2f), ForceMultiplier=%.2f"),
			   OwnerMass, TargetMass, OwnerRatio, TargetRatio, ForceMultiplier);
		UE_LOG(LogLasso, Log, TEXT("  VelocityChange magnitude=%.1f (per frame), Direction=%s"),
			   VelocityChange.Size(), *Direction.ToString());
	}

	// Apply velocity to owner (toward target)
	if (OwnerMovement)
	{
		FVector OwnerVelocityAdd = VelocityChange * OwnerRatio;
		FVector CurrentVelocity = OwnerMovement->Velocity;
		// Add velocity in the pull direction, preserving some existing velocity
		FVector NewVelocity = CurrentVelocity + OwnerVelocityAdd;
		OwnerMovement->Velocity = NewVelocity;

		if (LassoTickLogCounter == 0)
		{
			UE_LOG(LogLasso, Log, TEXT("  Owner velocity: %s -> %s (added %s)"),
				   *CurrentVelocity.ToString(), *NewVelocity.ToString(), *OwnerVelocityAdd.ToString());
		}
	}

	// Apply velocity to target (toward owner) - use LaunchCharacter for AI-controlled characters
	if (TargetChar && TargetMovement)
	{
		FVector TargetVelocityAdd = -VelocityChange * TargetRatio;

		// For AI characters, use LaunchCharacter which bypasses AI movement overrides
		// XYOverride=false means add to existing, ZOverride=false means add to existing
		TargetChar->LaunchCharacter(TargetVelocityAdd, false, false);

		if (LassoTickLogCounter == 0)
		{
			UE_LOG(LogLasso, Log, TEXT("  Target LaunchCharacter: %s (velocity add toward owner)"),
				   *TargetVelocityAdd.ToString());
		}
	}
}

void ALasso::UpdateCableVisual()
{
	if (!RopeCable)
	{
		return;
	}

	if (CurrentState == ELassoState::Throwing && ActiveProjectile)
	{
		// Cable start: hand/weapon position
		RopeCable->SetWorldLocation(OwnerCharacter->GetActorLocation() + FVector(0, 0, 50));

		// Cable end: connect to visual loop mesh on projectile
		FVector EndPoint = ActiveProjectile->GetActorLocation();
		if (UStaticMeshComponent *LoopMesh = ActiveProjectile->GetRopeLoopMesh())
		{
			EndPoint = LoopMesh->GetComponentLocation();
		}

		RopeCable->EndLocation = RopeCable->GetComponentTransform().InverseTransformPosition(EndPoint);

		// Set length to distance so it's taut/straight during throw
		float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), EndPoint);
		RopeCable->CableLength = Distance;
	}
	else if (CurrentState == ELassoState::Tethered && TetheredTarget && OwnerCharacter)
	{
		// Cable start: hand/weapon position
		RopeCable->SetWorldLocation(OwnerCharacter->GetActorLocation() + FVector(0, 0, 50));

		// Cable end: connect to loop mesh if available, otherwise target
		FVector EndPoint = TetheredTarget->GetActorLocation();
		if (SpawnedLoopMesh)
		{
			EndPoint = SpawnedLoopMesh->GetActorLocation();
		}

		RopeCable->EndLocation = RopeCable->GetComponentTransform().InverseTransformPosition(EndPoint);

		// Adjust cable length based on distance
		float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), EndPoint);
		RopeCable->CableLength = Distance;
	}
}

void ALasso::SpawnProjectile(const FVector &Location, const FVector &Direction)
{
	if (!ProjectileClass || !GetWorld())
	{
		UE_LOG(LogLasso, Error, TEXT("Lasso::SpawnProjectile - FAILED: ProjectileClass=%s, World=%s"),
			   ProjectileClass ? TEXT("valid") : TEXT("null"), GetWorld() ? TEXT("valid") : TEXT("null"));
		return;
	}

	UE_LOG(LogLasso, Log, TEXT("Lasso::SpawnProjectile - Spawning at %s, direction %s"),
		   *Location.ToString(), *Direction.ToString());

	ActiveProjectile = GetWorld()->SpawnActor<ALassoProjectile>(
		ProjectileClass,
		Location,
		Direction.Rotation());

	if (ActiveProjectile)
	{
		ActiveProjectile->SetOwner(OwnerCharacter);
		ActiveProjectile->SetLassoWeapon(this);
		ActiveProjectile->Launch(Direction);

		// Make cable visible immediately
		RopeCable->SetVisibility(true);

		UE_LOG(LogLasso, Log, TEXT("Lasso::SpawnProjectile - SUCCESS: Projectile %s launched"),
			   *GetNameSafe(ActiveProjectile));
	}
	else
	{
		UE_LOG(LogLasso, Error, TEXT("Lasso::SpawnProjectile - FAILED: SpawnActor returned null"));
	}
}

void ALasso::DestroyProjectile()
{
	if (ActiveProjectile)
	{
		ActiveProjectile->Destroy();
		ActiveProjectile = nullptr;
	}
}

void ALasso::SpawnLoopMeshOnTarget(AActor *Target)
{
	if (!Target || !LoopMeshClass || !GetWorld())
	{
		UE_LOG(LogLasso, Warning, TEXT("Lasso::SpawnLoopMeshOnTarget - FAILED: Target=%s, LoopMeshClass=%s, World=%s"),
			   *GetNameSafe(Target), LoopMeshClass ? TEXT("valid") : TEXT("null"), GetWorld() ? TEXT("valid") : TEXT("null"));
		return;
	}

	UE_LOG(LogLasso, Log, TEXT("Lasso::SpawnLoopMeshOnTarget - Spawning loop on %s"), *GetNameSafe(Target));

	// Get attachment transform from LassoableComponent if available
	FTransform SpawnTransform = FTransform::Identity;

	// Try procedural wrap first
	if (ULassoableComponent *Lassoable = Target->FindComponentByClass<ULassoableComponent>())
	{
		if (Lassoable->WrapLoopActorClass && Lassoable->WrapSpline)
		{
			// Spawn procedural loop
			FTransform SplineTransform = Lassoable->WrapSpline->GetComponentTransform();
			SpawnedLoopMesh = GetWorld()->SpawnActor<AActor>(Lassoable->WrapLoopActorClass, SplineTransform);

			if (ALassoLoopActor *LoopActor = Cast<ALassoLoopActor>(SpawnedLoopMesh))
			{
				LoopActor->AttachToComponent(Lassoable->WrapSpline, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				LoopActor->InitFromSpline(Lassoable->WrapSpline);
				UE_LOG(LogLasso, Log, TEXT("Lasso::SpawnLoopMeshOnTarget - Procedural loop spawned on %s"), *GetNameSafe(Target));
				return;
			}
		}

		// If no procedural setup, check for simple attachment transform
		SpawnTransform = Lassoable->GetLoopAttachTransform();
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::SpawnLoopMeshOnTarget - Using LassoableComponent transform: %s"),
			   *SpawnTransform.ToString());
	}
	else
	{
		// Fallback: spawn at target location
		SpawnTransform.SetLocation(Target->GetActorLocation());
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::SpawnLoopMeshOnTarget - No LassoableComponent, using target location"));
	}

	// Spawn the simple loop mesh actor (Fallback)
	if (LoopMeshClass)
	{
		SpawnedLoopMesh = GetWorld()->SpawnActor<AActor>(LoopMeshClass, SpawnTransform);

		if (SpawnedLoopMesh)
		{
			// CRITICAL: Disable ALL collision on the loop mesh to prevent it from pushing the target
			SpawnedLoopMesh->SetActorEnableCollision(false);

			// Also disable collision on all primitive components as a safeguard
			TArray<UPrimitiveComponent *> PrimitiveComponents;
			SpawnedLoopMesh->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
			for (UPrimitiveComponent *PrimComp : PrimitiveComponents)
			{
				PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(LogLasso, Verbose, TEXT("  Disabled collision on component: %s"), *GetNameSafe(PrimComp));
			}

			// Attach to target so it follows
			SpawnedLoopMesh->AttachToActor(Target, FAttachmentTransformRules::KeepWorldTransform);
			UE_LOG(LogLasso, Log, TEXT("Lasso::SpawnLoopMeshOnTarget - Simple loop spawned on %s (collision disabled)"), *GetNameSafe(Target));
		}
		else
		{
			UE_LOG(LogLasso, Error, TEXT("Lasso::SpawnLoopMeshOnTarget - FAILED to spawn loop mesh actor"));
		}
	}
}

void ALasso::DestroyLoopMesh()
{
	if (SpawnedLoopMesh)
	{
		UE_LOG(LogLasso, Verbose, TEXT("Lasso::DestroyLoopMesh - Destroying %s"), *GetNameSafe(SpawnedLoopMesh));
		SpawnedLoopMesh->Destroy();
		SpawnedLoopMesh = nullptr;
	}
}
