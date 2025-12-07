#pragma once

#include "CoreMinimal.h"
#include "GECattleBase.h"
#include "GECattleGuide.generated.h"

/**
 * Applied when cattle enters a Guide Volume.
 * Grants 'State.Cattle.Guided'.
 */
UCLASS()
class CATTLEGAME_API UGECattleGuide : public UGECattleBase
{
	GENERATED_BODY()

public:
	UGECattleGuide();
};
