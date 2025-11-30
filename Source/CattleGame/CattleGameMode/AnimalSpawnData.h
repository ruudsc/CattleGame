// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIController.h"
#include "AnimalSpawnData.generated.h"

/**
 * UAnimalSpawnData
 *
 * A Data Asset that defines animal spawn configuration for the GameMode.
 * Contains the actor blueprint to spawn, its AI controller, and spawn count.
 */
UCLASS(BlueprintType)
class CATTLEGAME_API UAnimalSpawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Blueprint class of the actor to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn Config")
	TSubclassOf<AActor> ActorToSpawn;

	/** AI Controller class to use for the spawned actor */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn Config")
	TSubclassOf<AAIController> AIControllerClass;

	/** Number of actors to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn Config", meta = (ClampMin = 1))
	int32 SpawnCount = 1;
};
