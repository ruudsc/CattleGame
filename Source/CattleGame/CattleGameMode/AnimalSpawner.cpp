// Copyright CattleGame. All Rights Reserved.

#include "AnimalSpawner.h"
#include "AnimalSpawnData.h"
#include "CattleGameState.h"
#include "Engine/World.h"

AAnimalSpawner::AAnimalSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bNetLoadOnClient = true;
}

void AAnimalSpawner::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("AAnimalSpawner::BeginPlay called. NetMode: %d"), (int32)GetWorld()->GetNetMode());

	// Only spawn on server
	if (GetWorld()->IsNetMode(NM_Client))
	{
		UE_LOG(LogTemp, Warning, TEXT("AAnimalSpawner: Running on client, skipping spawn"));
		return;
	}

	// Server spawns animals and notifies clients
	UE_LOG(LogTemp, Warning, TEXT("AAnimalSpawner: Running on server, spawning animals..."));
	SpawnAnimals();
	NotifyGameStateOfSpawnData();
}

const TArray<UAnimalSpawnData*>& AAnimalSpawner::GetAnimalSpawnDataArray() const
{
	return AnimalSpawnDataArray;
}

void AAnimalSpawner::SpawnAnimals()
{
	if (AnimalSpawnDataArray.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AAnimalSpawner: AnimalSpawnDataArray is empty. No animals will be spawned."));
		return;
	}

	int32 TotalSpawned = 0;

	for (const UAnimalSpawnData* SpawnData : AnimalSpawnDataArray)
	{
		if (!SpawnData)
		{
			UE_LOG(LogTemp, Warning, TEXT("AAnimalSpawner: Found null SpawnData in array. Skipping."));
			continue;
		}

		if (!SpawnData->ActorToSpawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("AAnimalSpawner: SpawnData has no ActorToSpawn class. Skipping."));
			continue;
		}

		// Spawn the configured number of animals
		for (int32 i = 0; i < SpawnData->SpawnCount; ++i)
		{
			FVector SpawnLocation = GetActorLocation() + FVector(FMath::RandRange(-500.0f, 500.0f), FMath::RandRange(-500.0f, 500.0f), 0.0f);
			FRotator SpawnRotation = FRotator::ZeroRotator;

			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
				SpawnData->ActorToSpawn,
				SpawnLocation,
				SpawnRotation
			);

			if (SpawnedActor)
			{
				TotalSpawned++;

				// Possess with AI controller if specified
				if (SpawnData->AIControllerClass)
				{
					APawn* SpawnedPawn = Cast<APawn>(SpawnedActor);
					if (SpawnedPawn)
					{
						AAIController* AIController = GetWorld()->SpawnActor<AAIController>(SpawnData->AIControllerClass);
						if (AIController)
						{
							AIController->Possess(SpawnedPawn);
						}
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AAnimalSpawner: Successfully spawned %d animals"), TotalSpawned);

	UE_LOG(LogTemp, Log, TEXT("AAnimalSpawner: Spawned %d animals from %d spawn configurations"), TotalSpawned, AnimalSpawnDataArray.Num());
}

void AAnimalSpawner::NotifyGameStateOfSpawnData()
{
	ACattleGameState* GameState = GetWorld()->GetGameState<ACattleGameState>();
	if (!GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("AAnimalSpawner: GameState not found or not of type ACattleGameState"));
		return;
	}

	// Copy spawn data to GameState for client replication
	GameState->AnimalSpawnDataArray = AnimalSpawnDataArray;
	UE_LOG(LogTemp, Log, TEXT("AAnimalSpawner: Notified GameState with %d spawn configurations"), AnimalSpawnDataArray.Num());
}
