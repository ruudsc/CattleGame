#include "Lasso.h"
#include "LassoProjectile.h"
#include "AbilitySystemComponent.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "CattleGame/CattleGame.h"

ALasso::ALasso()
{
	WeaponSlotID = 1; // Lasso is in slot 1
	WeaponName = FString(TEXT("Lasso"));
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f; // Tick every frame for smooth physics

	// Create scene root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Create lasso mesh component and attach to root
	LassoMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LassoMesh"));
	LassoMeshComponent->SetupAttachment(RootComponent);
	LassoMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALasso::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle cooldown (runs on all instances)
	TickCooldown(DeltaTime);

	// State-specific tick logic (server authoritative)
	if (HasAuthority())
	{
		switch (CurrentState)
		{
		case ELassoState::InFlight:
			TickInFlight(DeltaTime);
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
}

// ===== WEAPON INTERFACE =====

bool ALasso::CanFire() const
{
	// Can only fire if lasso is idle and not in cooldown
	if (CurrentState != ELassoState::Idle)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::CanFire - FALSE: State is %d (not Idle)"), (int32)CurrentState);
		return false;
	}

	if (CooldownRemaining > 0.0f)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::CanFire - FALSE: Cooldown remaining %.2f"), CooldownRemaining);
		return false;
	}

	return true;
}

void ALasso::ReleaseTether()
{
	if (CurrentState != ELassoState::Tethered)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerReleaseTether();
		return;
	}

	UE_LOG(LogGASDebug, Log, TEXT("Lasso::ReleaseTether - Releasing tethered target"));

	// Clear tethered target
	TetheredTarget = nullptr;
	bIsPulling = false;

	// Notify blueprint
	OnLassoReleased();

	// Start retracting
	StartRetract();
}

void ALasso::StartRetract()
{
	if (CurrentState == ELassoState::Idle || CurrentState == ELassoState::Retracting)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerStartRetract();
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("Lasso::StartRetract - Beginning retract"));

	// Clear any tethered target
	TetheredTarget = nullptr;
	bIsPulling = false;

	// Tell projectile to start retracting
	if (LassoProjectile)
	{
		LassoProjectile->StartRetract();
	}

	// Transition to retracting state
	SetState(ELassoState::Retracting);

	// Notify blueprint
	OnLassoRetractStarted();
}

void ALasso::ForceReset()
{
	UE_LOG(LogGASDebug, Warning, TEXT("Lasso::ForceReset - Force resetting to Idle state (was state %d)"), (int32)CurrentState);

	// Destroy the projectile if it exists
	if (LassoProjectile)
	{
		LassoProjectile->Destroy();
		LassoProjectile = nullptr;
	}

	// Clear all state
	TetheredTarget = nullptr;
	bIsPulling = false;
	CooldownRemaining = 0.0f;

	// Force to idle state
	SetState(ELassoState::Idle);
}

// ===== PROJECTILE CALLBACKS =====

void ALasso::OnProjectileHitTarget(AActor *Target)
{
	if (!HasAuthority() || CurrentState != ELassoState::InFlight)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::OnProjectileHitTarget - Ignored (Auth=%d, State=%d)"), HasAuthority(), (int32)CurrentState);
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("Lasso::OnProjectileHitTarget - Caught %s"), *GetNameSafe(Target));

	// Store tethered target
	TetheredTarget = Target;

	// Calculate resting rope length (distance at catch time)
	if (OwnerCharacter && Target)
	{
		RestingRopeLength = FVector::Dist(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
		CurrentRopeLength = RestingRopeLength;
	}

	// Transition to tethered state
	SetState(ELassoState::Tethered);

	// Notify blueprint
	OnLassoCaughtTarget(Target);
}

void ALasso::OnProjectileMissed()
{
	if (!HasAuthority() || CurrentState != ELassoState::InFlight)
	{
		return;
	}

	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnProjectileMissed - Starting retract"));

	// Start retracting
	StartRetract();
}

void ALasso::OnRetractComplete()
{
	if (!HasAuthority() || CurrentState != ELassoState::Retracting)
	{
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("Lasso::OnRetractComplete - Retract finished, starting cooldown"));

	// Notify Blueprint - projectile cycle is complete (can clean up VFX, hide projectile mesh, etc.)
	OnProjectileDestroyed();

	// Start cooldown
	CooldownRemaining = ThrowCooldown;

	// Transition to idle
	SetState(ELassoState::Idle);

	// Notify blueprint - retract is complete
	OnLassoRetractComplete();

	// Notify blueprint - weapon should be re-attached to hand socket
	OnLassoReattached();
}

// ===== PHYSICS HELPERS =====

float ALasso::GetActorMass(AActor *Actor)
{
	if (!Actor)
	{
		return 100.0f; // Default mass
	}

	// Try to get mass from CharacterMovementComponent
	if (ACharacter *Character = Cast<ACharacter>(Actor))
	{
		if (UCharacterMovementComponent *MovementComp = Character->GetCharacterMovement())
		{
			return MovementComp->Mass;
		}
	}

	// Try to get mass from physics body
	if (UPrimitiveComponent *RootPrim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		if (RootPrim->IsSimulatingPhysics())
		{
			return RootPrim->GetMass();
		}
	}

	return 100.0f; // Default mass
}

void ALasso::UpdateCharacterAbilityTag(AActor *Character, const FGameplayTag &Tag, bool bAdd)
{
	if (!Character)
	{
		return;
	}

	IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Character);
	if (!ASI)
	{
		return;
	}

	UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (bAdd)
	{
		ASC->AddLooseGameplayTag(Tag);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(Tag);
	}
}

void ALasso::SetPulling(bool bPulling)
{
	if (bIsPulling == bPulling)
	{
		return;
	}

	bIsPulling = bPulling;

	if (bIsPulling)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::SetPulling - Pull started"));
		OnPullStarted();
	}
	else
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::SetPulling - Pull stopped"));
		OnPullStopped();
	}
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
	// Update gameplay tags on clients when state replicates
	// Note: This is a simplified version - full tag update happens on server
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnRep_CurrentState - State changed to %d"), (int32)CurrentState);
}

// ===== INTERNAL STATE MANAGEMENT =====

void ALasso::SetState(ELassoState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	ELassoState OldState = CurrentState;
	CurrentState = NewState;

	// Update gameplay tags
	UpdateStateTags(OldState, NewState);

	// Update weapon visibility based on state
	UpdateWeaponVisibility(OldState, NewState);

	UE_LOG(LogGASDebug, Warning, TEXT("Lasso::SetState - %d -> %d"), (int32)OldState, (int32)NewState);

	// Notify Blueprint of state change
	OnLassoStateChanged(OldState, NewState);
}

void ALasso::UpdateWeaponVisibility(ELassoState OldState, ELassoState NewState)
{
	// Hide weapon mesh when lasso is thrown (entering InFlight from Idle)
	if (OldState == ELassoState::Idle && NewState == ELassoState::InFlight)
	{
		SetWeaponMeshVisible(false);
	}
	// Show weapon mesh when lasso returns to idle (retract complete)
	else if (NewState == ELassoState::Idle)
	{
		SetWeaponMeshVisible(true);
	}

	// Handle rope wrap visuals
	if (NewState == ELassoState::Tethered && TetheredTarget)
	{
		// Show rope wrap on tethered target
		OnShowRopeWrapVisual(TetheredTarget);
	}
	else if (OldState == ELassoState::Tethered && TetheredTarget)
	{
		// Hide rope wrap when leaving tethered state
		OnHideRopeWrapVisual(TetheredTarget);
	}
}

void ALasso::UpdateStateTags(ELassoState OldState, ELassoState NewState)
{
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	if (!CurrentOwner)
	{
		return;
	}

	// Remove old state tags
	switch (OldState)
	{
	case ELassoState::InFlight:
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Throwing, false);
		break;
	case ELassoState::Tethered:
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Pulling, false);
		break;
	case ELassoState::Retracting:
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Retracting, false);
		break;
	default:
		break;
	}

	// Add new state tags
	switch (NewState)
	{
	case ELassoState::Idle:
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Active, false);
		break;
	case ELassoState::InFlight:
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Active, true);
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Throwing, true);
		break;
	case ELassoState::Tethered:
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Active, true);
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Pulling, true);
		break;
	case ELassoState::Retracting:
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Active, true);
		UpdateCharacterAbilityTag(CurrentOwner, CattleGameplayTags::State_Lasso_Retracting, true);
		break;
	}
}

// ===== TICK HANDLERS =====

void ALasso::TickInFlight(float DeltaTime)
{
	// Projectile handles its own flight logic and will call OnProjectileHitTarget or OnProjectileMissed
}

void ALasso::TickTethered(float DeltaTime)
{
	if (!TetheredTarget || !OwnerCharacter)
	{
		// Target was destroyed or became invalid
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::TickTethered - Lost target! TetheredTarget=%s, OwnerCharacter=%s"),
			   TetheredTarget ? TEXT("Valid") : TEXT("NULL"),
			   OwnerCharacter ? TEXT("Valid") : TEXT("NULL"));

		// Notify Blueprint about lost target
		if (TetheredTarget)
		{
			OnTargetLost(TetheredTarget);
		}

		StartRetract();
		return;
	}

	// Update current rope length
	CurrentRopeLength = FVector::Dist(OwnerCharacter->GetActorLocation(), TetheredTarget->GetActorLocation());

	// Apply elastic tether force if pulling
	if (bIsPulling)
	{
		ApplyElasticTetherForce(DeltaTime);
	}
}

void ALasso::TickRetracting(float DeltaTime)
{
	// Projectile handles retraction and will call OnRetractComplete when done
	// Check if projectile is close enough to complete retraction
	if (LassoProjectile && OwnerCharacter)
	{
		float DistanceToOwner = FVector::Dist(LassoProjectile->GetActorLocation(), OwnerCharacter->GetActorLocation());

		// Log progress every second or so
		static float LogTimer = 0.0f;
		LogTimer += DeltaTime;
		if (LogTimer > 1.0f)
		{
			UE_LOG(LogGASDebug, Warning, TEXT("Lasso::TickRetracting - Distance to owner: %.1f, Projectile retracting: %s"),
				   DistanceToOwner, LassoProjectile->IsRetracting() ? TEXT("YES") : TEXT("NO"));
			LogTimer = 0.0f;
		}

		if (DistanceToOwner < 100.0f)
		{
			UE_LOG(LogGASDebug, Warning, TEXT("Lasso::TickRetracting - Projectile reached owner, completing retract"));
			OnRetractComplete();
		}
	}
	else
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::TickRetracting - Missing projectile or owner! Forcing complete."));
		OnRetractComplete();
	}
}

void ALasso::TickCooldown(float DeltaTime)
{
	if (CooldownRemaining > 0.0f)
	{
		CooldownRemaining -= DeltaTime;
		if (CooldownRemaining <= 0.0f)
		{
			CooldownRemaining = 0.0f;
			OnLassoReady();
		}
	}
}

// ===== PHYSICS =====

void ALasso::ApplyElasticTetherForce(float DeltaTime)
{
	if (!TetheredTarget || !OwnerCharacter)
	{
		return;
	}

	// Calculate stretch (difference from resting length)
	float Stretch = CurrentRopeLength - RestingRopeLength;

	// Only apply force if rope is stretched beyond resting length
	if (Stretch <= 0.0f)
	{
		// Notify Blueprint with zero tension
		OnRopeTensionChanged(0.0f, 0.0f);
		return;
	}

	// Calculate spring force: F = -k * x - c * v (spring + damping)
	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = TetheredTarget->GetActorLocation();
	FVector RopeDirection = (TargetLocation - OwnerLocation).GetSafeNormal();

	float SpringForce = RopeElasticity * Stretch;

	// Notify Blueprint of rope tension (for VFX like cable thickness, particles, etc.)
	OnRopeTensionChanged(Stretch, SpringForce);

	// Get masses for force distribution
	float OwnerMass = GetActorMass(OwnerCharacter);
	float TargetMass = GetActorMass(TetheredTarget);
	float TotalMass = OwnerMass + TargetMass;

	// Force ratio: heavier entity moves less
	float OwnerForceRatio = TargetMass / TotalMass; // Owner gets pulled more if target is heavier
	float TargetForceRatio = OwnerMass / TotalMass; // Target gets pulled more if owner is heavier

	// Apply force to owner (toward target)
	if (UCharacterMovementComponent *OwnerMovement = OwnerCharacter->GetCharacterMovement())
	{
		FVector ForceOnOwner = RopeDirection * SpringForce * OwnerForceRatio;
		OwnerMovement->AddForce(ForceOnOwner);
	}

	// Apply force to target (toward owner)
	if (ACharacter *TargetCharacter = Cast<ACharacter>(TetheredTarget))
	{
		if (UCharacterMovementComponent *TargetMovement = TargetCharacter->GetCharacterMovement())
		{
			FVector ForceOnTarget = -RopeDirection * SpringForce * TargetForceRatio;
			TargetMovement->AddForce(ForceOnTarget);
		}
	}
}

// ===== SERVER RPCS =====

void ALasso::ServerFire_Implementation(const FVector_NetQuantize &SpawnLocation, const FVector_NetQuantizeNormal &LaunchDirection)
{
	if (!CanFire())
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Lasso::ServerFire - Cannot fire (state=%d, cooldown=%.2f)"),
			   (int32)CurrentState, CooldownRemaining);
		return;
	}

	if (!GetWorld() || !ProjectileClass || !OwnerCharacter)
	{
		UE_LOG(LogGASDebug, Error, TEXT("Lasso::ServerFire - Missing world, projectile class, or owner"));
		return;
	}

	const FRotator SpawnRotation = LaunchDirection.Rotation();

	// Spawn or reuse projectile
	if (!LassoProjectile)
	{
		LassoProjectile = GetWorld()->SpawnActor<ALassoProjectile>(
			ProjectileClass,
			SpawnLocation,
			SpawnRotation);

		if (!LassoProjectile)
		{
			UE_LOG(LogGASDebug, Error, TEXT("Lasso::ServerFire - Failed to spawn projectile"));
			return;
		}

		LassoProjectile->SetOwner(OwnerCharacter);
		LassoProjectile->SetLassoWeapon(this);
	}
	else
	{
		// Reset projectile for reuse
		LassoProjectile->SetActorLocation(SpawnLocation);
		LassoProjectile->SetActorRotation(SpawnRotation);
	}

	// Configure and launch projectile
	LassoProjectile->SetLassoProperties(ThrowSpeed, MaxRopeLength, GravityScale, AimAssistAngle, AimAssistMaxDistance, RetractSpeed);
	LassoProjectile->Launch(LaunchDirection.GetSafeNormal(), ThrowSpeed);

	// Transition to in-flight state
	SetState(ELassoState::InFlight);

	// Notify blueprint
	OnLassoThrown();

	UE_LOG(LogGASDebug, Warning, TEXT("Lasso::ServerFire - Projectile launched, State=%d"), (int32)CurrentState);
}

void ALasso::ServerReleaseTether_Implementation()
{
	ReleaseTether();
}

void ALasso::ServerStartRetract_Implementation()
{
	StartRetract();
}

// ===== VISIBILITY =====

void ALasso::SetWeaponMeshVisible(bool bVisible)
{
	if (LassoMeshComponent)
	{
		LassoMeshComponent->SetVisibility(bVisible, true);
	}
}

// ===== BLUEPRINT NATIVE EVENT IMPLEMENTATIONS =====

void ALasso::OnLassoThrown_Implementation()
{
	// Default C++ implementation: hide the lasso mesh when thrown
	// (visibility is already handled by UpdateWeaponVisibility, but log for debugging)
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoThrown_Implementation - Projectile launched"));
}

void ALasso::OnLassoCaughtTarget_Implementation(AActor *CaughtTarget)
{
	// Default C++ implementation: show rope wrap visual on target
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoCaughtTarget_Implementation - Caught %s"), *GetNameSafe(CaughtTarget));
}

void ALasso::OnLassoReleased_Implementation()
{
	// Default C++ implementation: cleanup when target is released
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoReleased_Implementation - Target released"));
}

void ALasso::OnLassoRetractStarted_Implementation()
{
	// Default C++ implementation: retract is starting
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoRetractStarted_Implementation - Retract started"));
}

void ALasso::OnLassoRetractComplete_Implementation()
{
	// Default C++ implementation: retract is complete
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoRetractComplete_Implementation - Retract complete"));
}

void ALasso::OnLassoReady_Implementation()
{
	// Default C++ implementation: lasso is ready to throw again
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoReady_Implementation - Ready to throw"));
}

void ALasso::OnShowRopeWrapVisual_Implementation(AActor *Target)
{
	// Default C++ implementation: placeholder for rope wrap visual
	// Blueprints can override to spawn particle systems, decals, etc.
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnShowRopeWrapVisual_Implementation - Target: %s"), *GetNameSafe(Target));
}

void ALasso::OnHideRopeWrapVisual_Implementation(AActor *Target)
{
	// Default C++ implementation: placeholder for hiding rope wrap
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnHideRopeWrapVisual_Implementation - Target: %s"), *GetNameSafe(Target));
}

void ALasso::OnLassoStateChanged_Implementation(ELassoState OldState, ELassoState NewState)
{
	// Default C++ implementation: log state changes
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoStateChanged_Implementation - %d -> %d"), (int32)OldState, (int32)NewState);
}

void ALasso::OnPullStarted_Implementation()
{
	// Default C++ implementation: player started pulling
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnPullStarted_Implementation - Pull started"));
}

void ALasso::OnPullStopped_Implementation()
{
	// Default C++ implementation: player stopped pulling
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnPullStopped_Implementation - Pull stopped"));
}

void ALasso::OnRopeTensionChanged_Implementation(float Stretch, float Force)
{
	// Default C++ implementation: rope tension changed (for VFX updates)
	// Only log occasionally to avoid spam
}

void ALasso::OnTargetLost_Implementation(AActor *LostTarget)
{
	// Default C++ implementation: target was lost (destroyed, disconnected)
	UE_LOG(LogGASDebug, Warning, TEXT("Lasso::OnTargetLost_Implementation - Lost target: %s"), *GetNameSafe(LostTarget));
}

void ALasso::OnLassoReattached_Implementation()
{
	// Default C++ implementation: re-attach to character hand after retract
	AttachToCharacterHand();
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnLassoReattached_Implementation - Reattached to character hand"));
}

void ALasso::OnProjectileDestroyed_Implementation()
{
	// Default C++ implementation: projectile cleanup
	UE_LOG(LogGASDebug, Log, TEXT("Lasso::OnProjectileDestroyed_Implementation - Projectile cycle complete"));
}
