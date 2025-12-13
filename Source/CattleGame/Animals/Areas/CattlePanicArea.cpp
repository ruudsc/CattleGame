// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattlePanicArea.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"

ACattlePanicArea::ACattlePanicArea()
{
    // Panic areas are highest priority
    Priority = 100;

    // Set debug color to red for panic
    DebugColor = FColor::Red;

    // Default smaller size for panic areas (point threats)
    if (BoxComponent)
    {
        BoxComponent->SetBoxExtent(FVector(300.0f, 300.0f, 250.0f));
    }

    // Larger edge falloff for gradual panic
    EdgeFalloff = 400.0f;
}

FVector ACattlePanicArea::GetInfluenceDirection(const FVector &Location) const
{
    return GetFleeDirection(Location) * FleeStrength;
}

FVector ACattlePanicArea::GetFleeDirection(const FVector &Location) const
{
    if (bRandomFleeDirection)
    {
        // Random direction for stampede chaos
        const float RandomAngle = FMath::FRand() * 360.0f;
        return FVector(FMath::Cos(FMath::DegreesToRadians(RandomAngle)), FMath::Sin(FMath::DegreesToRadians(RandomAngle)), 0.0f);
    }

    FVector ThreatCenter;

    if (bUseSplineShape && SplineComponent)
    {
        // Find closest point on spline as threat source
        ThreatCenter = SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
    }
    else if (BoxComponent)
    {
        ThreatCenter = BoxComponent->GetComponentLocation();
    }
    else
    {
        ThreatCenter = GetActorLocation();
    }

    // Direction away from threat
    FVector FleeDir = Location - ThreatCenter;
    FleeDir.Z = 0.0f; // Keep horizontal

    // If at center, pick a random direction
    if (FleeDir.IsNearlyZero())
    {
        const float RandomAngle = FMath::FRand() * 360.0f;
        return FVector(FMath::Cos(FMath::DegreesToRadians(RandomAngle)), FMath::Sin(FMath::DegreesToRadians(RandomAngle)), 0.0f);
    }

    return FleeDir.GetSafeNormal();
}
