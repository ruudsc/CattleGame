#pragma once

#include "CoreMinimal.h"
#include "CattleVolumeLogicComponent.h"
#include "CattleGuideLogic.generated.h"

class USplineComponent;

/**
 * DEPRECATED: Use ACattleGuideSpline actor instead.
 * Logic Component that directs Cattle along a Spline.
 * User must add a USplineComponent to the same Volume Actor.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DeprecatedNode, DeprecationMessage = "Use ACattleGuideSpline actor instead"))
class CATTLEGAME_API UCattleGuideLogic : public UCattleVolumeLogicComponent
{
	GENERATED_BODY()

public:
	UCattleGuideLogic();

	virtual void BeginPlay() override;

	// Override to provide Spline-based flow
	virtual FVector GetFlowDirection(const FVector &Location) const override;

protected:
	/** Finds the SplineComponent on the owner */
	void CacheSpline();

private:
	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> CachedSpline;
};
