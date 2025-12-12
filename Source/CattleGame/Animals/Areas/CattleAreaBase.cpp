// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAreaBase.h"
#include "CattleAreaSubsystem.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

ACattleAreaBase::ACattleAreaBase()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // Create spline component for freeform shapes
    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    SplineComponent->SetupAttachment(SceneRoot);
    SplineComponent->SetClosedLoop(true);

    // Create box component for simple shapes
    BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
    BoxComponent->SetupAttachment(SceneRoot);
    BoxComponent->SetBoxExtent(FVector(500.0f, 500.0f, 250.0f));
    BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoxComponent->SetGenerateOverlapEvents(false);

#if WITH_EDITORONLY_DATA
    // Make spline visible in editor
    SplineComponent->SetDrawDebug(true);
    SplineComponent->bShouldVisualizeScale = true;
#endif
}

void ACattleAreaBase::BeginPlay()
{
    Super::BeginPlay();
    RegisterWithSubsystem();
}

void ACattleAreaBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromSubsystem();
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void ACattleAreaBase::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Update component visibility based on shape mode
    if (SplineComponent)
    {
        SplineComponent->SetVisibility(bUseSplineShape);
    }
    if (BoxComponent)
    {
        BoxComponent->SetVisibility(!bUseSplineShape);
        BoxComponent->SetBoxExtent(FVector(BoxComponent->GetUnscaledBoxExtent().X, BoxComponent->GetUnscaledBoxExtent().Y, AreaHeight * 0.5f));
    }
}
#endif

bool ACattleAreaBase::IsLocationInArea(const FVector &Location) const
{
    // Check height first
    const float ZMin = GetActorLocation().Z - AreaHeight * 0.5f;
    const float ZMax = GetActorLocation().Z + AreaHeight * 0.5f;
    if (Location.Z < ZMin || Location.Z > ZMax)
    {
        return false;
    }

    if (bUseSplineShape)
    {
        return IsInsideSplineArea(Location);
    }
    else
    {
        // Use box bounds
        if (BoxComponent)
        {
            const FVector LocalLocation = BoxComponent->GetComponentTransform().InverseTransformPosition(Location);
            const FVector Extent = BoxComponent->GetUnscaledBoxExtent();
            return FMath::Abs(LocalLocation.X) <= Extent.X &&
                   FMath::Abs(LocalLocation.Y) <= Extent.Y &&
                   FMath::Abs(LocalLocation.Z) <= Extent.Z;
        }
    }

    return false;
}

FCattleAreaInfluence ACattleAreaBase::GetInfluenceAtLocation(const FVector &Location) const
{
    FCattleAreaInfluence Influence;

    if (!IsLocationInArea(Location))
    {
        return Influence;
    }

    Influence.AreaType = GetAreaType();
    Influence.AreaActor = const_cast<ACattleAreaBase *>(this);
    Influence.InfluenceDirection = GetInfluenceDirection(Location);
    Influence.SpeedModifier = GetSpeedModifier();
    Influence.Priority = Priority + static_cast<int32>(GetAreaType()); // Add area type to priority for proper ordering

    const float DistToBoundary = GetDistanceToBoundary(Location);
    Influence.Strength = CalculateInfluenceStrength(DistToBoundary);

    return Influence;
}

FVector ACattleAreaBase::GetInfluenceDirection(const FVector &Location) const
{
    // Default: no direction (override in subclasses)
    return FVector::ZeroVector;
}

void ACattleAreaBase::DrawDebugArea(float Duration) const
{
    UWorld *World = GetWorld();
    if (!World)
        return;

    if (bUseSplineShape && SplineComponent)
    {
        // Draw spline points
        const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
        for (int32 i = 0; i < NumPoints; ++i)
        {
            const FVector CurrentPoint = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            const FVector NextPoint = SplineComponent->GetLocationAtSplinePoint((i + 1) % NumPoints, ESplineCoordinateSpace::World);

            DrawDebugLine(World, CurrentPoint, NextPoint, DebugColor, false, Duration, 0, 5.0f);
            DrawDebugSphere(World, CurrentPoint, 25.0f, 8, DebugColor, false, Duration);
        }
    }
    else if (BoxComponent)
    {
        DrawDebugBox(World, BoxComponent->GetComponentLocation(), BoxComponent->GetScaledBoxExtent(), BoxComponent->GetComponentQuat(), DebugColor, false, Duration, 0, 5.0f);
    }
}

float ACattleAreaBase::GetDistanceToBoundary(const FVector &Location) const
{
    if (bUseSplineShape && SplineComponent)
    {
        // Find closest point on spline and calculate distance
        const FVector ClosestPoint = SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
        const float Distance = FVector::Dist2D(Location, ClosestPoint);

        // Negative if inside (this is a simplification - proper polygon distance is more complex)
        return IsInsideSplineArea(Location) ? -Distance : Distance;
    }
    else if (BoxComponent)
    {
        // Distance to box boundary
        const FVector LocalLocation = BoxComponent->GetComponentTransform().InverseTransformPosition(Location);
        const FVector Extent = BoxComponent->GetUnscaledBoxExtent();

        // Calculate how far inside/outside each axis
        FVector Distances;
        Distances.X = FMath::Abs(LocalLocation.X) - Extent.X;
        Distances.Y = FMath::Abs(LocalLocation.Y) - Extent.Y;
        Distances.Z = FMath::Abs(LocalLocation.Z) - Extent.Z;

        // If all negative, we're inside - return most negative (deepest)
        if (Distances.X < 0 && Distances.Y < 0 && Distances.Z < 0)
        {
            return FMath::Max3(Distances.X, Distances.Y, Distances.Z);
        }

        // Outside - return positive distance
        return FVector(FMath::Max(0.0f, Distances.X), FMath::Max(0.0f, Distances.Y), FMath::Max(0.0f, Distances.Z)).Size();
    }

    return 0.0f;
}

float ACattleAreaBase::CalculateInfluenceStrength(float DistanceToBoundary) const
{
    if (DistanceToBoundary >= 0.0f)
    {
        // Outside the area
        return 0.0f;
    }

    // Inside the area
    const float DistanceInside = -DistanceToBoundary;

    if (EdgeFalloff <= 0.0f)
    {
        return 1.0f;
    }

    // Linear falloff at edges
    return FMath::Clamp(DistanceInside / EdgeFalloff, 0.0f, 1.0f);
}

bool ACattleAreaBase::IsInsideSplineArea(const FVector &Location) const
{
    if (!SplineComponent || SplineComponent->GetNumberOfSplinePoints() < 3)
    {
        return false;
    }

    // Use ray casting algorithm to determine if point is inside closed spline polygon
    // Cast a ray in the +X direction and count intersections
    int32 Intersections = 0;
    const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
    const float TestZ = Location.Z;

    // Sample the spline at regular intervals for better accuracy
    const int32 SamplesPerSegment = 10;
    const int32 TotalSamples = NumPoints * SamplesPerSegment;

    for (int32 i = 0; i < TotalSamples; ++i)
    {
        const float T1 = static_cast<float>(i) / TotalSamples;
        const float T2 = static_cast<float>(i + 1) / TotalSamples;

        const FVector P1 = SplineComponent->GetLocationAtTime(T1, ESplineCoordinateSpace::World, true);
        const FVector P2 = SplineComponent->GetLocationAtTime(T2, ESplineCoordinateSpace::World, true);

        // Check if edge crosses the horizontal ray from Location in +X direction
        if ((P1.Y <= Location.Y && P2.Y > Location.Y) || (P2.Y <= Location.Y && P1.Y > Location.Y))
        {
            // Calculate X intersection
            const float T = (Location.Y - P1.Y) / (P2.Y - P1.Y);
            const float IntersectX = P1.X + T * (P2.X - P1.X);

            if (IntersectX > Location.X)
            {
                Intersections++;
            }
        }
    }

    // Odd number of intersections = inside
    return (Intersections % 2) == 1;
}

UCattleAreaSubsystem *ACattleAreaBase::GetAreaSubsystem() const
{
    if (UWorld *World = GetWorld())
    {
        return World->GetSubsystem<UCattleAreaSubsystem>();
    }
    return nullptr;
}

void ACattleAreaBase::RegisterWithSubsystem()
{
    if (UCattleAreaSubsystem *Subsystem = GetAreaSubsystem())
    {
        Subsystem->RegisterArea(this);
    }
}

void ACattleAreaBase::UnregisterFromSubsystem()
{
    if (UCattleAreaSubsystem *Subsystem = GetAreaSubsystem())
    {
        Subsystem->UnregisterArea(this);
    }
}
