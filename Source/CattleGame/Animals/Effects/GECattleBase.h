#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GECattleBase.generated.h"

/**
 * Base Gameplay Effect for Cattle interactions.
 */
UCLASS()
class CATTLEGAME_API UGECattleBase : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGECattleBase();
};
