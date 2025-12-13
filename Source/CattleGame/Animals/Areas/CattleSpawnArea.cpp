// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleSpawnArea.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"

ACattleSpawnArea::ACattleSpawnArea()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // Create spline component for freeform shapes
    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    SplineComponent->SetupAttachment(SceneRoot);
    SplineComponent->SetClosedLoop(true);

    // Set up default spline points (square)
    SplineComponent->ClearSplinePoints();
    SplineComponent->AddSplinePoint(FVector(-500.0f, -500.0f, 0.0f), ESplineCoordinateSpace::Local);
    SplineComponent->AddSplinePoint(FVector(500.0f, -500.0f, 0.0f), ESplineCoordinateSpace::Local);
    SplineComponent->AddSplinePoint(FVector(500.0f, 500.0f, 0.0f), ESplineCoordinateSpace::Local);
    SplineComponent->AddSplinePoint(FVector(-500.0f, 500.0f, 0.0f), ESplineCoordinateSpace::Local);

    // Create box component for simple shapes
    BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
    BoxComponent->SetupAttachment(SceneRoot);
    BoxComponent->SetBoxExtent(FVector(500.0f, 500.0f, 100.0f));
    BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoxComponent->SetGenerateOverlapEvents(false);

#if WITH_EDITORONLY_DATA
    // Create billboard for editor visibility
    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(SceneRoot);

    SplineComponent->SetDrawDebug(true);
#endif
}

void ACattleSpawnArea::BeginPlay()
{
    Super::BeginPlay();

    if (bSpawnOnBeginPlay && HasAuthority())
    {
        SpawnAllAnimals();
    }
}

void ACattleSpawnArea::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
    UpdateShapeVisibility();
}

#if WITH_EDITOR
void ACattleSpawnArea::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        if (PropertyName == GET_MEMBER_NAME_CHECKED(ACattleSpawnArea, bUseSplineShape))
        {
            UpdateShapeVisibility();
        }
    }
}
#endif

void ACattleSpawnArea::SpawnAllAnimals()
{
    if (!HasAuthority())
    {
        return;
    }

    // Clear previous spawns
    SpawnedAnimals.Empty();
    UsedSpawnLocations.Empty();

    // Calculate total spawn count for even distribution
    const int32 TotalCount = GetTotalSpawnCount();
    if (TotalCount == 0)
    {
        return;
    }

    // Generate evenly distributed spawn points
    TArray<FVector> SpawnPoints = GenerateEvenlyDistributedPoints(TotalCount);

    int32 PointIndex = 0;
    for (const FCattleSpawnItem &SpawnItem : SpawnItems)
    {
        if (!SpawnItem.ActorBlueprint)
        {
            continue;
        }

        for (int32 i = 0; i < SpawnItem.SpawnCount && PointIndex < SpawnPoints.Num(); ++i, ++PointIndex)
        {
            FVector SpawnLocation = SpawnPoints[PointIndex];

            // Perform ground trace to place on navmesh/ground
            FHitResult HitResult;
            FVector TraceStart = SpawnLocation + FVector(0.0f, 0.0f, 500.0f);
            FVector TraceEnd = SpawnLocation - FVector(0.0f, 0.0f, 1000.0f);

            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(this);

            if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
            {
                SpawnLocation = HitResult.Location + FVector(0.0f, 0.0f, SpawnHeightOffset);
            }

            // Spawn the animal
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            ACattleAnimal *SpawnedAnimal = GetWorld()->SpawnActor<ACattleAnimal>(
                SpawnItem.ActorBlueprint,
                SpawnLocation,
                FRotator(0.0f, FMath::FRand() * 360.0f, 0.0f),
                SpawnParams);

            if (SpawnedAnimal)
            {
                SpawnedAnimals.Add(SpawnedAnimal);
                UsedSpawnLocations.Add(SpawnLocation);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("CattleSpawnArea: Spawned %d animals"), SpawnedAnimals.Num());
}

TArray<ACattleAnimal *> ACattleSpawnArea::SpawnAnimalsOfType(const FCattleSpawnItem &SpawnItem)
{
    TArray<ACattleAnimal *> Result;

    if (!SpawnItem.ActorBlueprint || !HasAuthority())
    {
        return Result;
    }

    for (int32 i = 0; i < SpawnItem.SpawnCount; ++i)
    {
        FVector SpawnLocation = FVector::ZeroVector;
        bool bFoundLocation = false;

        // Try to find a valid spawn location
        for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
        {
            FVector TestLocation = GetRandomSpawnLocation();
            if (IsLocationFarEnoughFromOthers(TestLocation))
            {
                SpawnLocation = TestLocation;
                bFoundLocation = true;
                break;
            }
        }

        if (!bFoundLocation)
        {
            // Fall back to any random location
            SpawnLocation = GetRandomSpawnLocation();
        }

        // Spawn the animal
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ACattleAnimal *SpawnedAnimal = GetWorld()->SpawnActor<ACattleAnimal>(
            SpawnItem.ActorBlueprint,
            SpawnLocation,
            FRotator(0.0f, FMath::FRand() * 360.0f, 0.0f),
            SpawnParams);

        if (SpawnedAnimal)
        {
            Result.Add(SpawnedAnimal);
            SpawnedAnimals.Add(SpawnedAnimal);
            UsedSpawnLocations.Add(SpawnLocation);
        }
    }

    return Result;
}

FVector ACattleSpawnArea::GetRandomSpawnLocation() const
{
    if (bUseSplineShape)
    {
        return GetRandomPointInSpline();
    }
    else
    {
        return GetRandomPointInBox();
    }
}

bool ACattleSpawnArea::IsValidSpawnLocation(const FVector &Location) const
{
    if (bUseSplineShape)
    {
        return IsInsideSplineArea(Location);
    }
    else
    {
        return IsInsideBoxArea(Location);
    }
}

int32 ACattleSpawnArea::GetTotalSpawnCount() const
{
    int32 Total = 0;
    for (const FCattleSpawnItem &Item : SpawnItems)
    {
        if (Item.ActorBlueprint)
        {
            Total += Item.SpawnCount;
        }
    }
    return Total;
}

TArray<ACattleAnimal *> ACattleSpawnArea::GetSpawnedAnimalsArray() const
{
    TArray<ACattleAnimal *> Result;
    for (const TWeakObjectPtr<ACattleAnimal> &WeakAnimal : SpawnedAnimals)
    {
        if (ACattleAnimal *Animal = WeakAnimal.Get())
        {
            Result.Add(Animal);
        }
    }
    return Result;
}

void ACattleSpawnArea::DrawDebugSpawnArea(float Duration) const
{
    UWorld *World = GetWorld();
    if (!World)
    {
        return;
    }

    if (bUseSplineShape && SplineComponent)
    {
        // Draw spline
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
        DrawDebugBox(World, BoxComponent->GetComponentLocation(), BoxComponent->GetScaledBoxExtent(),
                     BoxComponent->GetComponentQuat(), DebugColor, false, Duration, 0, 5.0f);
    }

    // Draw spawn locations
    for (const FVector &Location : UsedSpawnLocations)
    {
        DrawDebugSphere(World, Location, 30.0f, 8, FColor::Green, false, Duration);
    }
}

bool ACattleSpawnArea::IsInsideSplineArea(const FVector &Location) const
{
    if (!SplineComponent || SplineComponent->GetNumberOfSplinePoints() < 3)
    {
        return false;
    }

    // Use ray casting algorithm
    int32 Intersections = 0;
    const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
    const int32 SamplesPerSegment = 10;
    const int32 TotalSamples = NumPoints * SamplesPerSegment;

    for (int32 i = 0; i < TotalSamples; ++i)
    {
        const float T1 = static_cast<float>(i) / TotalSamples;
        const float T2 = static_cast<float>(i + 1) / TotalSamples;

        const FVector P1 = SplineComponent->GetLocationAtTime(T1, ESplineCoordinateSpace::World, true);
        const FVector P2 = SplineComponent->GetLocationAtTime(T2, ESplineCoordinateSpace::World, true);

        if ((P1.Y <= Location.Y && P2.Y > Location.Y) || (P2.Y <= Location.Y && P1.Y > Location.Y))
        {
            const float T = (Location.Y - P1.Y) / (P2.Y - P1.Y);
            const float IntersectX = P1.X + T * (P2.X - P1.X);

            if (IntersectX > Location.X)
            {
                Intersections++;
            }
        }
    }

    return (Intersections % 2) == 1;
}

bool ACattleSpawnArea::IsInsideBoxArea(const FVector &Location) const
{
    if (!BoxComponent)
    {
        return false;
    }

    const FVector LocalLocation = BoxComponent->GetComponentTransform().InverseTransformPosition(Location);
    const FVector Extent = BoxComponent->GetUnscaledBoxExtent();

    return FMath::Abs(LocalLocation.X) <= Extent.X &&
           FMath::Abs(LocalLocation.Y) <= Extent.Y &&
           FMath::Abs(LocalLocation.Z) <= Extent.Z;
}

FVector ACattleSpawnArea::GetRandomPointInBox() const
{
    if (!BoxComponent)
    {
        return GetActorLocation();
    }

    const FVector Extent = BoxComponent->GetScaledBoxExtent();
    const FVector RandomLocal(
        FMath::FRandRange(-Extent.X, Extent.X),
        FMath::FRandRange(-Extent.Y, Extent.Y),
        0.0f);

    return BoxComponent->GetComponentTransform().TransformPosition(RandomLocal);
}

FVector ACattleSpawnArea::GetRandomPointInSpline() const
{
    if (!SplineComponent || SplineComponent->GetNumberOfSplinePoints() < 3)
    {
        return GetActorLocation();
    }

    // Calculate bounding box of spline
    FBox SplineBounds(ForceInit);
    const int32 NumSamples = 50;
    for (int32 i = 0; i <= NumSamples; ++i)
    {
        const float T = static_cast<float>(i) / NumSamples;
        SplineBounds += SplineComponent->GetLocationAtTime(T, ESplineCoordinateSpace::World, true);
    }

    // Try random points until we find one inside
    for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
    {
        FVector TestPoint(
            FMath::FRandRange(SplineBounds.Min.X, SplineBounds.Max.X),
            FMath::FRandRange(SplineBounds.Min.Y, SplineBounds.Max.Y),
            GetActorLocation().Z);

        if (IsInsideSplineArea(TestPoint))
        {
            return TestPoint;
        }
    }

    // Fallback: return center of bounding box
    return SplineBounds.GetCenter();
}

bool ACattleSpawnArea::IsLocationFarEnoughFromOthers(const FVector &Location) const
{
    for (const FVector &UsedLocation : UsedSpawnLocations)
    {
        if (FVector::Dist2D(Location, UsedLocation) < MinSpawnDistance)
        {
            return false;
        }
    }
    return true;
}

TArray<FVector> ACattleSpawnArea::GenerateEvenlyDistributedPoints(int32 Count) const
{
    TArray<FVector> Points;

    if (Count <= 0)
    {
        return Points;
    }

    if (bUseSplineShape)
    {
        // For spline, use rejection sampling with Poisson disk distribution approximation
        if (!SplineComponent || SplineComponent->GetNumberOfSplinePoints() < 3)
        {
            return Points;
        }

        // Calculate bounding box
        FBox SplineBounds(ForceInit);
        const int32 NumSamples = 50;
        for (int32 i = 0; i <= NumSamples; ++i)
        {
            const float T = static_cast<float>(i) / NumSamples;
            SplineBounds += SplineComponent->GetLocationAtTime(T, ESplineCoordinateSpace::World, true);
        }

        // Calculate ideal spacing based on area and count
        const FVector BoundsSize = SplineBounds.GetSize();
        const float ApproxArea = BoundsSize.X * BoundsSize.Y * 0.7f; // Assume ~70% fill
        const float IdealSpacing = FMath::Sqrt(ApproxArea / Count) * 0.8f;
        const float ActualSpacing = FMath::Max(IdealSpacing, MinSpawnDistance);

        TArray<FVector> TempUsedLocations;

        for (int32 i = 0; i < Count; ++i)
        {
            bool bFound = false;

            for (int32 Attempt = 0; Attempt < MaxSpawnAttempts * 2; ++Attempt)
            {
                FVector TestPoint(
                    FMath::FRandRange(SplineBounds.Min.X, SplineBounds.Max.X),
                    FMath::FRandRange(SplineBounds.Min.Y, SplineBounds.Max.Y),
                    GetActorLocation().Z);

                if (IsInsideSplineArea(TestPoint))
                {
                    bool bFarEnough = true;
                    for (const FVector &UsedPoint : TempUsedLocations)
                    {
                        if (FVector::Dist2D(TestPoint, UsedPoint) < ActualSpacing)
                        {
                            bFarEnough = false;
                            break;
                        }
                    }

                    if (bFarEnough)
                    {
                        Points.Add(TestPoint);
                        TempUsedLocations.Add(TestPoint);
                        bFound = true;
                        break;
                    }
                }
            }

            // If we couldn't find a spaced location, just find any valid location
            if (!bFound)
            {
                for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
                {
                    FVector TestPoint(
                        FMath::FRandRange(SplineBounds.Min.X, SplineBounds.Max.X),
                        FMath::FRandRange(SplineBounds.Min.Y, SplineBounds.Max.Y),
                        GetActorLocation().Z);

                    if (IsInsideSplineArea(TestPoint))
                    {
                        Points.Add(TestPoint);
                        TempUsedLocations.Add(TestPoint);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // For box, use grid distribution
        if (!BoxComponent)
        {
            return Points;
        }

        const FVector Extent = BoxComponent->GetScaledBoxExtent();
        const float AreaWidth = Extent.X * 2.0f;
        const float AreaHeight = Extent.Y * 2.0f;

        // Calculate grid dimensions
        const float Ratio = AreaWidth / AreaHeight;
        const int32 Rows = FMath::Max(1, FMath::RoundToInt(FMath::Sqrt(Count / Ratio)));
        const int32 Cols = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Count) / Rows));

        const float CellWidth = AreaWidth / Cols;
        const float CellHeight = AreaHeight / Rows;

        int32 PointsGenerated = 0;
        for (int32 Row = 0; Row < Rows && PointsGenerated < Count; ++Row)
        {
            for (int32 Col = 0; Col < Cols && PointsGenerated < Count; ++Col)
            {
                // Calculate cell center with some randomization
                float X = -Extent.X + (Col + 0.5f) * CellWidth + FMath::FRandRange(-CellWidth * 0.3f, CellWidth * 0.3f);
                float Y = -Extent.Y + (Row + 0.5f) * CellHeight + FMath::FRandRange(-CellHeight * 0.3f, CellHeight * 0.3f);

                // Clamp to ensure we stay within bounds
                X = FMath::Clamp(X, -Extent.X + 50.0f, Extent.X - 50.0f);
                Y = FMath::Clamp(Y, -Extent.Y + 50.0f, Extent.Y - 50.0f);

                FVector LocalPoint(X, Y, 0.0f);
                FVector WorldPoint = BoxComponent->GetComponentTransform().TransformPosition(LocalPoint);

                Points.Add(WorldPoint);
                PointsGenerated++;
            }
        }
    }

    return Points;
}

void ACattleSpawnArea::UpdateShapeVisibility()
{
    if (SplineComponent)
    {
        SplineComponent->SetVisibility(bUseSplineShape);
    }
    if (BoxComponent)
    {
        BoxComponent->SetVisibility(!bUseSplineShape);
    }
}
