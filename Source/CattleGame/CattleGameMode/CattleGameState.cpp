// Copyright CattleGame. All Rights Reserved.

#include "CattleGameState.h"
#include "Net/UnrealNetwork.h"

ACattleGameState::ACattleGameState()
{
}

const TArray<UAnimalSpawnData*>& ACattleGameState::GetAnimalSpawnDataArray() const
{
	return AnimalSpawnDataArray;
}

void ACattleGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACattleGameState, AnimalSpawnDataArray);
}
