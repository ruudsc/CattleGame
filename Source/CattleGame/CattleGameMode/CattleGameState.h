// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CattleGameState.generated.h"

class UAnimalSpawnData;

/**
 * ACattleGameState
 *
 * Server-authoritative game state replicated to all clients.
 * Contains level-specific spawn configuration that clients need for UI and gameplay mechanics.
 */
UCLASS()
class CATTLEGAME_API ACattleGameState : public AGameState
{
	GENERATED_BODY()

public:
	ACattleGameState();

public:
	/** Animal spawn data array replicated from server to all clients */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Spawn System")
	TArray<UAnimalSpawnData*> AnimalSpawnDataArray;

	/**
	 * Get the animal spawn data array (safe accessor for clients)
	 * @return Reference to the animal spawn data array
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spawn")
	const TArray<UAnimalSpawnData*>& GetAnimalSpawnDataArray() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
