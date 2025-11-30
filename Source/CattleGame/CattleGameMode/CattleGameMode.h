// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "CattleGameMode.generated.h"

/**
 * ACattleGameMode
 *
 * Main game mode for CattleGame.
 * Sets up the custom GameState class. Animal spawning is handled by AAnimalSpawner in the level.
 */
UCLASS()
class CATTLEGAME_API ACattleGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ACattleGameMode();

protected:
	/** Choose a PlayerStart for a new player. Distributes players across available starts; if only one start exists, falls back to offset spawning. */
	virtual AActor *ChoosePlayerStart_Implementation(AController *Player) override;

	/** After spawning at PlayerStart, apply offset if necessary to avoid overlap when only one PlayerStart is present. */
	virtual void RestartPlayerAtPlayerStart(AController *NewPlayer, AActor *StartSpot) override;

private:
	/** Incrementing index for round-robin PlayerStart selection. */
	int32 NextSpawnIndex = 0;

	/** Horizontal spacing between players when fanning out from a single start. */
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float SingleStartHorizontalSpacing = 200.f;

	/** Radial spacing when using radial pattern (if > 0 uses circle). */
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float SingleStartRadialRadius = 0.f; // 0 disables radial pattern
};
