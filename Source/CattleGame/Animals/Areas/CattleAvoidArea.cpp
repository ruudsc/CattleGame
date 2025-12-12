// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAvoidArea.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"

ACattleAvoidArea::ACattleAvoidArea()
{
    // Avoid areas are medium priority
    Priority = 50;

    // Set debug color to orange for avoid
    DebugColor = FColor::Orange;

    // Default size for avoid areas
    if (BoxComponent)
    {
        BoxComponent->SetBoxExtent(FVector(200.0f, 200.0f, 250.0f));
    }

    // Sharp edge falloff for clear boundaries
    EdgeFalloff = 100.0f;
}

bool ACattleAvoidArea::IsLocationInArea(const FVector &Location) const
{
    // Avoid areas extend their influence beyond their actual boundary
    // Check if within avoidance radius
    const float Dist = GetDistanceToBoundary(Location);

    // Inside the actual area or within avoidance radius outside
    return Dist < AvoidanceRadius;
}

FCattleAreaInfluence ACattleAvoidArea::GetInfluenceAtLocation(const FVector &Location) const
{
    FCattleAreaInfluence Influence;

    const float DistToBoundary = GetDistanceToBoundary(Location);

    // Check if within avoidance range (including outside the boundary)
    if (DistToBoundary >= AvoidanceRadius)
    {
        return Influence; // Too far away
    }

    // Check height
    const float ZMin = GetActorLocation().Z - AreaHeight * 0.5f;
    const float ZMax = GetActorLocation().Z + AreaHeight * 0.5f;
    if (Location.Z < ZMin || Location.Z > ZMax)
    {
        return Influence;
    }

    Influence.AreaType = GetAreaType();
    Influence.AreaActor = const_cast<ACattleAvoidArea *>(this);
    Influence.InfluenceDirection = GetAvoidanceDirection(Location);
    Influence.Priority = Priority + static_cast<int32>(GetAreaType());

    // Calculate strength based on distance
    if (DistToBoundary < 0)
    {
        // Inside the area - full strength
        Influence.Strength = 1.0f;
        Influence.SpeedModifier = InsideSpeedModifier;
    }
    else
    {
        // Outside but within avoidance radius - falloff
        Influence.Strength = 1.0f - (DistToBoundary / AvoidanceRadius);
        Influence.SpeedModifier = 1.0f; // Normal speed outside
    }

    return Influence;
}

FVector ACattleAvoidArea::GetInfluenceDirection(const FVector &Location) const
{
    return GetAvoidanceDirection(Location) * AvoidStrength;
}

FVector ACattleAvoidArea::GetAvoidanceDirection(const FVector &Location) const
{
    FVector ClosestBoundaryPoint;

    if (bUseSplineShape && SplineComponent)
    {
        ClosestBoundaryPoint = SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
    }
    else if (BoxComponent)
    {
        // Find closest point on box surface
        const FVector LocalLocation = BoxComponent->GetComponentTransform().InverseTransformPosition(Location);
        const FVector Extent = BoxComponent->GetUnscaledBoxExtent();

        // Clamp to box surface
        FVector ClampedLocal;
        ClampedLocal.X = FMath::Clamp(LocalLocation.X, -Extent.X, Extent.X);
        ClampedLocal.Y = FMath::Clamp(LocalLocation.Y, -Extent.Y, Extent.Y);
        ClampedLocal.Z = FMath::Clamp(LocalLocation.Z, -Extent.Z, Extent.Z);

        // If inside, push to nearest face
        if (LocalLocation.X >= -Extent.X && LocalLocation.X <= Extent.X &&
            LocalLocation.Y >= -Extent.Y && LocalLocation.Y <= Extent.Y &&
            LocalLocation.Z >= -Extent.Z && LocalLocation.Z <= Extent.Z)
        {
            // Find which face is closest
            float MinDist = Extent.X - FMath::Abs(LocalLocation.X);
            int32 ClosestAxis = 0;

            if (Extent.Y - FMath::Abs(LocalLocation.Y) < MinDist)
            {
                MinDist = Extent.Y - FMath::Abs(LocalLocation.Y);
                ClosestAxis = 1;
            }
            if (Extent.Z - FMath::Abs(LocalLocation.Z) < MinDist)
            {
                ClosestAxis = 2;
            }

            // Move to closest face
            switch (ClosestAxis)
            {
            case 0:
                ClampedLocal.X = (LocalLocation.X >= 0) ? Extent.X : -Extent.X;
                break;
            case 1:
                ClampedLocal.Y = (LocalLocation.Y >= 0) ? Extent.Y : -Extent.Y;
                break;
            case 2:
                ClampedLocal.Z = (LocalLocation.Z >= 0) ? Extent.Z : -Extent.Z;
                break;
            }
        }

        ClosestBoundaryPoint = BoxComponent->GetComponentTransform().TransformPosition(ClampedLocal);
    }
    else
    {
        ClosestBoundaryPoint = GetActorLocation();
    }

    // Direction away from boundary
    FVector AwayDir = Location - ClosestBoundaryPoint;
    AwayDir.Z = 0.0f; // Keep horizontal

    if (AwayDir.IsNearlyZero())
    {
        // At boundary center, push in a consistent direction
        return GetActorForwardVector();
    }

    return AwayDir.GetSafeNormal();
}
