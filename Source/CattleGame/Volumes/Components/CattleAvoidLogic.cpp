#include "CattleAvoidLogic.h"

FVector UCattleAvoidLogic::GetFlowDirection(const FVector& Location) const
{
	if (AActor* Owner = GetOwner())
	{
		FVector Center = Owner->GetActorLocation();
		FVector Direction = (Location - Center).GetSafeNormal();
		
		// Push AWAY from center
		return Direction;
	}
	return FVector::ZeroVector;
}
