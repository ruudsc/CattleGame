#pragma once

#include "CoreMinimal.h"
#include "GECattleBase.h"
#include "GECattleAvoid.generated.h"

/**
 * Applied when cattle enters an Avoid Volume.
 * Grants 'State.Cattle.Avoiding'.
 */
UCLASS()
class CATTLEGAME_API UGECattleAvoid : public UGECattleBase
{
	GENERATED_BODY()

public:
	UGECattleAvoid();
};
