#pragma once

#include "CoreMinimal.h"
#include "CattleVolumeLogicComponent.h"
#include "CattleAvoidLogic.generated.h"

/**
 * DEPRECATED: Use ACattleAvoidVolume actor instead.
 * Logic Component that repels Cattle from the Volume center.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DeprecatedNode, DeprecationMessage = "Use ACattleAvoidVolume actor instead"))
class CATTLEGAME_API UCattleAvoidLogic : public UCattleVolumeLogicComponent
{
	GENERATED_BODY()

public:
	virtual FVector GetFlowDirection(const FVector &Location) const override;
};
