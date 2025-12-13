// Copyright CattleGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CattleGameState.generated.h"

/**
 * ACattleGameState
 *
 * Server-authoritative game state replicated to all clients.
 */
UCLASS()
class CATTLEGAME_API ACattleGameState : public AGameState
{
	GENERATED_BODY()

public:
	ACattleGameState();
};
