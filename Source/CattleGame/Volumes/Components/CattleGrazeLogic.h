#pragma once

#include "CoreMinimal.h"
#include "CattleVolumeLogicComponent.h"
#include "CattleGrazeLogic.generated.h"

/**
 * DEPRECATED: Use ACattleGrazeVolume actor instead.
 * Logic Component that marks an area as Grazable.
 * Does not provide flow (returns Zero).
 * Check for the Applied GameplayEffect to know if cattle is grazing.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DeprecatedNode, DeprecationMessage = "Use ACattleGrazeVolume actor instead"))
class CATTLEGAME_API UCattleGrazeLogic : public UCattleVolumeLogicComponent
{
	GENERATED_BODY()
};
