#include "Trumpet.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CattleGame/CattleGame.h"

ATrumpet::ATrumpet()
{
	WeaponSlotID = 3; // Trumpet is in slot 3
	WeaponName = FString(TEXT("Trumpet"));
	bReplicates = true;

	// Create scene root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Create trumpet mesh component and attach to root
	TrumpetMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrumpetMesh"));
	TrumpetMeshComponent->SetupAttachment(RootComponent);
	TrumpetMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATrumpet::EquipWeapon()
{
	// Call base to fire event
	Super::EquipWeapon();

	// Attach to character hand socket
	AttachToCharacterHand();

	UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::EquipWeapon - Attached to character hand"));
}

void ATrumpet::UnequipWeapon()
{
	// Stop any playing sounds
	StopPlaying();

	// Detach from character
	// DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Call base to fire event
	Super::UnequipWeapon();

	UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::UnequipWeapon - Detached from character"));
}

void ATrumpet::Fire()
{
	// Call base function to fire blueprint event
	Super::Fire();

	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;

	if (CurrentOwner && HasAuthority() && !bIsPlaying)
	{
		PlayLure();
		UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::Fire - Playing Lure"));
	}

	// Fire the blueprint event
	OnWeaponFired(CurrentOwner, ActiveMesh);
}

bool ATrumpet::CanFire() const
{
	// Trumpet can always be played if equipped
	return true;
}

void ATrumpet::PlayScare()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsPlaying && bIsPlayingLure)
	{
		// Already playing Lure, switch to Scare
		bIsPlayingLure = false;
		UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::PlayScare - Switched from Lure to Scare"));
	}
	else if (!bIsPlaying)
	{
		// Not playing, start Scare
		bIsPlaying = true;
		bIsPlayingLure = false;
		OnTrumpetStarted.Broadcast();
		UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::PlayScare - Playing Scare"));
	}
}

void ATrumpet::StopPlaying()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bIsPlaying)
	{
		return;
	}

	bIsPlaying = false;
	bIsPlayingLure = false;
	OnTrumpetStopped.Broadcast();

	UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::StopPlaying - Trumpet stopped"));
}

void ATrumpet::PlayLure()
{
	bIsPlaying = true;
	bIsPlayingLure = true;
	OnTrumpetStarted.Broadcast();

	UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::PlayLure - Playing Lure"));
}

void ATrumpet::GetAffectedTargets(TArray<AActor *> &OutTargets)
{
	if (!GetOwner())
	{
		return;
	}

	OutTargets.Empty();

	// Get effective radius based on current mode
	float Radius = bIsPlayingLure ? LureRadius : ScareRadius;
	FVector EffectCenter = GetOwner()->GetActorLocation();

	// Sphere overlap query
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);
	TArray<FHitResult> HitResults;

	bool bHitSomething = GetWorld()->SweepMultiByChannel(
		HitResults,
		EffectCenter,
		EffectCenter,
		FQuat::Identity,
		ECC_Pawn,
		SphereShape);

	if (bHitSomething)
	{
		for (const FHitResult &HitResult : HitResults)
		{
			if (AActor *HitActor = HitResult.GetActor())
			{
				if (HitActor != GetOwner()) // Don't affect owner
				{
					OutTargets.Add(HitActor);
				}
			}
		}
	}
}

void ATrumpet::ApplyEffectForces()
{
	if (!bIsPlaying || !GetOwner())
	{
		return;
	}

	TArray<AActor *> AffectedTargets;
	GetAffectedTargets(AffectedTargets);

	FVector OwnerLocation = GetOwner()->GetActorLocation();
	float EffectStrength = bIsPlayingLure ? LureStrength : ScareStrength;

	for (AActor *Target : AffectedTargets)
	{
		if (!Target)
			continue;

		if (APawn *TargetPawn = Cast<APawn>(Target))
		{
			if (ACharacter *TargetCharacter = Cast<ACharacter>(TargetPawn))
			{
				if (UCharacterMovementComponent *MovementComponent = TargetCharacter->GetCharacterMovement())
				{
					FVector TargetLocation = Target->GetActorLocation();
					FVector EffectDirection;

					if (bIsPlayingLure)
					{
						// Lure: Pull toward player
						EffectDirection = (OwnerLocation - TargetLocation).GetSafeNormal();
					}
					else
					{
						// Scare: Repel away from player
						EffectDirection = (TargetLocation - OwnerLocation).GetSafeNormal();
					}

					// Apply force
					MovementComponent->AddForce(EffectDirection * EffectStrength);
				}
			}
		}
	}
}

void ATrumpet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrumpet, bIsPlaying);
	DOREPLIFETIME(ATrumpet, bIsPlayingLure);
}

void ATrumpet::AttachToCharacterHand()
{
	ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
	if (!CurrentOwner)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::AttachToCharacterHand - No owner character"));
		return;
	}

	USkeletalMeshComponent *TargetMesh = CurrentOwner->GetActiveCharacterMesh();
	if (!TargetMesh)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::AttachToCharacterHand - No active character mesh"));
		return;
	}

	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
	AttachToComponent(TargetMesh, AttachRules, TrumpetHandSocketName);

	UE_LOG(LogGASDebug, Log, TEXT("Trumpet::AttachToCharacterHand - Attached to socket %s on mesh %s"), *TrumpetHandSocketName.ToString(), *TargetMesh->GetName());
}
