#include "Trumpet.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/OverlapResult.h"
#include "CattleGame/CattleGame.h"

ATrumpet::ATrumpet()
{
	WeaponSlotID = 3; // Trumpet is in slot 3
	WeaponName = FString(TEXT("Trumpet"));
	bReplicates = true;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Create scene root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Create trumpet mesh component and attach to root
	TrumpetMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrumpetMesh"));
	TrumpetMeshComponent->SetupAttachment(RootComponent);
	TrumpetMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATrumpet::BeginPlay()
{
	Super::BeginPlay();
}

void ATrumpet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only process effects on server
	if (!HasAuthority() || !bIsPlaying)
	{
		return;
	}

	if (bIsPlayingLure)
	{
		ApplyLureEffects(DeltaTime);
	}
	else
	{
		ApplyScareEffects(DeltaTime);
	}
}

bool ATrumpet::CanFire() const
{
	// Trumpet can always be played if equipped
	return true;
}

void ATrumpet::PlayLure()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsPlaying && !bIsPlayingLure)
	{
		// Already playing Scare, switch to Lure
		bIsPlayingLure = true;
		UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::PlayLure - Switched from Scare to Lure"));
	}
	else if (!bIsPlaying)
	{
		// Not playing, start Lure
		bIsPlaying = true;
		bIsPlayingLure = true;
		OnTrumpetStarted.Broadcast();
		UE_LOG(LogGASDebug, Warning, TEXT("Trumpet::PlayLure - Playing Lure"));
	}
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

void ATrumpet::ApplyLureEffects(float DeltaTime)
{
	if (!OwnerCharacter)
	{
		return;
	}

	const FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	TArray<ACattleAnimal *> NearbyCattle = GetCattleInRadius(LureRadius);

	for (ACattleAnimal *Cattle : NearbyCattle)
	{
		// Always apply calm
		Cattle->AddCalm(CalmPerSecond * DeltaTime);

		// If calm enough, attract toward player
		if (Cattle->GetFearPercent() < LureAttractionThreshold)
		{
			FVector DirectionToPlayer = (PlayerLocation - Cattle->GetActorLocation()).GetSafeNormal();
			DirectionToPlayer.Z = 0.0f; // Keep horizontal

			// Apply gentle movement impulse toward player
			Cattle->ApplyPhysicsImpulse(DirectionToPlayer * LureAttractionSpeed * DeltaTime, true);
		}
	}
}

void ATrumpet::ApplyScareEffects(float DeltaTime)
{
	TArray<ACattleAnimal *> NearbyCattle = GetCattleInRadius(ScareRadius);

	for (ACattleAnimal *Cattle : NearbyCattle)
	{
		Cattle->AddFear(FearPerSecond * DeltaTime);
	}
}

TArray<ACattleAnimal *> ATrumpet::GetCattleInRadius(float Radius) const
{
	TArray<ACattleAnimal *> Result;

	if (!OwnerCharacter || !GetWorld())
	{
		return Result;
	}

	const FVector Center = OwnerCharacter->GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

	if (GetWorld()->OverlapMultiByChannel(
			OverlapResults,
			Center,
			FQuat::Identity,
			ECC_Pawn,
			SphereShape))
	{
		for (const FOverlapResult &Overlap : OverlapResults)
		{
			if (ACattleAnimal *Cattle = Cast<ACattleAnimal>(Overlap.GetActor()))
			{
				Result.Add(Cattle);
			}
		}
	}

	return Result;
}

void ATrumpet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrumpet, bIsPlaying);
	DOREPLIFETIME(ATrumpet, bIsPlayingLure);
}
