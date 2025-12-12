// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleGrazeArea.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"

ACattleGrazeArea::ACattleGrazeArea()
{
    // Graze areas are low priority
    Priority = 0;

    // Set debug color to green for graze
    DebugColor = FColor::Green;

    // Default size for graze areas
    if (BoxComponent)
    {
        BoxComponent->SetBoxExtent(FVector(1000.0f, 1000.0f, 250.0f));
    }
}

FVector ACattleGrazeArea::GetInfluenceDirection(const FVector &Location) const
{
    // Graze areas gently push animals toward center when near edges
    return GetContainmentDirection(Location) * ContainmentStrength;
}

FVector ACattleGrazeArea::GetContainmentDirection(const FVector &Location) const
{
    FVector AreaCenter;

    if (bUseSplineShape && SplineComponent)
    {
        // Calculate centroid of spline points
        AreaCenter = FVector::ZeroVector;
        const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
        for (int32 i = 0; i < NumPoints; ++i)
        {
            AreaCenter += SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        }
        if (NumPoints > 0)
        {
            AreaCenter /= NumPoints;
        }
    }
    else if (BoxComponent)
    {
        AreaCenter = BoxComponent->GetComponentLocation();
    }
    else
    {
        AreaCenter = GetActorLocation();
    }

    // Get distance to boundary
    const float DistToBoundary = GetDistanceToBoundary(Location);

    // Only apply containment force near edges (within falloff distance)
    if (EdgeFalloff > 0.0f && DistToBoundary > -EdgeFalloff)
    {
        // Calculate strength based on how close to edge
        const float EdgeProximity = 1.0f - FMath::Clamp(-DistToBoundary / EdgeFalloff, 0.0f, 1.0f);

        // Direction toward center
        FVector ToCenter = AreaCenter - Location;
        ToCenter.Z = 0.0f; // Keep horizontal

        return ToCenter.GetSafeNormal() * EdgeProximity;
    }

    return FVector::ZeroVector;
}
