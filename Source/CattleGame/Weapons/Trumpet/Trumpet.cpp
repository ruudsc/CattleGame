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

void ATrumpet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrumpet, bIsPlaying);
	DOREPLIFETIME(ATrumpet, bIsPlayingLure);
}
