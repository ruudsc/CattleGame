#include "CattleGuideLogic.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"

UCattleGuideLogic::UCattleGuideLogic()
{
}

void UCattleGuideLogic::BeginPlay()
{
	Super::BeginPlay();
	CacheSpline();
}

void UCattleGuideLogic::CacheSpline()
{
	if (AActor* Owner = GetOwner())
	{
		CachedSpline = Owner->FindComponentByClass<USplineComponent>();
	}
}

FVector UCattleGuideLogic::GetFlowDirection(const FVector& Location) const
{
	if (!CachedSpline)
	{
		// Try to cache if missed
		const_cast<UCattleGuideLogic*>(this)->CacheSpline();
	}

	if (CachedSpline)
	{
		// Find closest point on spline to the cattle
		// Then get the direction / tangent at that point
		FVector SplinePos = CachedSpline->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
		FVector Tangent = CachedSpline->FindTangentClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
		
		// Ideally we want to push them TO the spline AND along it?
		// For now, let's just push them ALONG the spline. The standard Tangent.
		return Tangent.GetSafeNormal();
	}

	return FVector::ZeroVector;
}
