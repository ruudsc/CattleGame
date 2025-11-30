// Copyright CattleGame. All Rights Reserved.

#include "CattleGameMode.h"
#include "CattleGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

ACattleGameMode::ACattleGameMode()
{
	GameStateClass = ACattleGameState::StaticClass();
}

AActor *ACattleGameMode::ChoosePlayerStart_Implementation(AController *Player)
{
	UWorld *World = GetWorld();
	if (!World)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	TArray<AActor *> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), PlayerStarts);

	if (PlayerStarts.Num() == 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// Round-robin selection if multiple starts exist
	if (PlayerStarts.Num() > 1)
	{
		AActor *Chosen = PlayerStarts[NextSpawnIndex % PlayerStarts.Num()];
		++NextSpawnIndex;
		return Chosen;
	}

	// Only one PlayerStart: just return it; offset handled post-spawn in RestartPlayerAtPlayerStart
	return PlayerStarts[0];
}

void ACattleGameMode::RestartPlayerAtPlayerStart(AController *NewPlayer, AActor *StartSpot)
{
	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);

	if (!NewPlayer || !StartSpot)
	{
		return;
	}

	// Only apply custom offset logic if we have exactly one PlayerStart in the world
	UWorld *World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor *> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), PlayerStarts);
	if (PlayerStarts.Num() != 1)
	{
		return; // Multiple starts case handled by ChoosePlayerStart
	}

	APawn *SpawnedPawn = NewPlayer->GetPawn();
	if (!SpawnedPawn)
	{
		return;
	}

	// Compute index based on number of existing players (PlayerState->GetPlayerId can be reused but may not be sequential if players leave)
	int32 PlayerIndex = 0;
	{
		// Count currently possessed pawns excluding this one for stable ordering
		int32 Count = 0;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController *PC = It->Get();
			if (PC && PC != NewPlayer && PC->GetPawn())
			{
				++Count;
			}
		}
		PlayerIndex = Count; // This pawn considered last
	}

	FVector BaseLocation = StartSpot->GetActorLocation();
	FVector Offset = FVector::ZeroVector;

	if (SingleStartRadialRadius > 0.f)
	{
		// Radial distribution
		const float AngleStep = 30.f * PI / 180.f; // 30 degree steps
		const float Angle = PlayerIndex * AngleStep;
		Offset.X = FMath::Cos(Angle) * SingleStartRadialRadius;
		Offset.Y = FMath::Sin(Angle) * SingleStartRadialRadius;
	}
	else
	{
		// Linear horizontal spacing along X axis (alternating left/right for compactness)
		const int32 RingIndex = PlayerIndex / 2;
		const bool bEven = (PlayerIndex % 2) == 0;
		float Direction = bEven ? 1.f : -1.f;
		Offset.Y = Direction * RingIndex * SingleStartHorizontalSpacing; // Spread along Y
	}

	FVector NewLocation = BaseLocation + Offset;
	FRotator NewRotation = SpawnedPawn->GetActorRotation();
	SpawnedPawn->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
}
