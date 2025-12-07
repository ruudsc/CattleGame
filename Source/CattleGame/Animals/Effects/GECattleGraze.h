#pragma once

#include "CoreMinimal.h"
#include "GECattleBase.h"
#include "GECattleGraze.generated.h"

/**
 * Applied when cattle enters a Grazing Volume.
 * Grants 'State.Cattle.Grazing'.
 */
UCLASS()
class CATTLEGAME_API UGECattleGraze : public UGECattleBase
{
	GENERATED_BODY()

public:
	UGECattleGraze();
};
