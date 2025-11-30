// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnimalSpawner.generated.h"

class UAnimalSpawnData;

/**
 * AAnimalSpawner
 *
 * Actor placed in level to configure and manage animal spawning per level.
 * Designers place this actor in the level and set the AnimalSpawnDataArray
 * property directly in the Details panel. The spawner handles:
 * - Spawning animals at game start based on configuration
 * - Notifying GameState so clients can access spawn data
 */
UCLASS()
class CATTLEGAME_API AAnimalSpawner : public AActor
{
	GENERATED_BODY()

public:
	AAnimalSpawner();

protected:
	virtual void BeginPlay() override;

public:
	/** Array of animal spawn data assets configured for this level */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Config")
	TArray<UAnimalSpawnData*> AnimalSpawnDataArray;

	/**
	 * Get the spawn data array for this level
	 * @return Reference to the animal spawn data array
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spawn")
	const TArray<UAnimalSpawnData*>& GetAnimalSpawnDataArray() const;

private:
	/** Spawn all animals based on AnimalSpawnDataArray configuration */
	void SpawnAnimals();

	/** Notify GameState about the spawn configuration for client access */
	void NotifyGameStateOfSpawnData();
};
