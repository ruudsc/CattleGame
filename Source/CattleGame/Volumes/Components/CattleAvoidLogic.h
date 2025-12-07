#pragma once

#include "CoreMinimal.h"
#include "CattleVolumeLogicComponent.h"
#include "CattleAvoidLogic.generated.h"

/**
 * Logic Component that repels Cattle from the Volume center.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CATTLEGAME_API UCattleAvoidLogic : public UCattleVolumeLogicComponent
{
	GENERATED_BODY()

public:
	virtual FVector GetFlowDirection(const FVector& Location) const override;
};
