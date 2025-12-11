#include "CattleGuideSpline.h"
#include "Components/SplineComponent.h"
#include "CattleFlowSubsystem.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

ACattleGuideSpline::ACattleGuideSpline()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true; // Always tick for proximity checks

    // Create spline component
    GuideSpline = CreateDefaultSubobject<USplineComponent>(TEXT("GuideSpline"));
    GuideSpline->SetupAttachment(RootSceneComponent);

    // Default spline setup - a simple straight path
    GuideSpline->ClearSplinePoints();
    GuideSpline->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
    GuideSpline->AddSplinePoint(FVector(1000, 0, 0), ESplineCoordinateSpace::Local);

    // Default debug color for guide splines
    DebugColor = FColor::Cyan;

    // Default influence radius for spline proximity
    InfluenceRadius = 500.0f;
}

void ACattleGuideSpline::BeginPlay()
{
    Super::BeginPlay();

    // Register with the flow subsystem for proximity queries
    if (UWorld *World = GetWorld())
    {
        if (UCattleFlowSubsystem *FlowSubsystem = World->GetSubsystem<UCattleFlowSubsystem>())
        {
            FlowSubsystem->RegisterProximitySource(this);
        }
    }
}

void ACattleGuideSpline::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up any active effects before unregistering
    for (auto &Pair : ActiveEffectHandles)
    {
        if (Pair.Key.IsValid())
        {
            UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pair.Key.Get());
            if (ASC)
            {
                ASC->RemoveActiveGameplayEffect(Pair.Value, 1);
            }
        }
    }
    ActiveEffectHandles.Empty();
    CattleInProximity.Empty();

    // Unregister from the flow subsystem
    if (UWorld *World = GetWorld())
    {
        if (UCattleFlowSubsystem *FlowSubsystem = World->GetSubsystem<UCattleFlowSubsystem>())
        {
            FlowSubsystem->UnregisterProximitySource(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void ACattleGuideSpline::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Periodic proximity check for GAS effect management
    ProximityCheckTimer += DeltaTime;
    if (ProximityCheckTimer >= ProximityCheckInterval)
    {
        ProximityCheckTimer = 0.0f;
        UpdateProximityCattle();
    }
}

FVector ACattleGuideSpline::GetFlowDirection_Implementation(const FVector &Location, float &OutWeight) const
{
    if (!GuideSpline)
    {
        OutWeight = 0.0f;
        return FVector::ZeroVector;
    }

    // Get distance from spline (not from actor center)
    FVector ClosestPoint;
    float Distance;
    GetClosestPointOnSpline(Location, ClosestPoint, Distance);

    // Calculate weight based on falloff from spline
    OutWeight = CalculateFalloffWeight(Distance);

    if (OutWeight <= 0.0f)
    {
        return FVector::ZeroVector;
    }

    return CalculateFlowDirectionInternal(Location);
}

FVector ACattleGuideSpline::CalculateFlowDirectionInternal(const FVector &Location) const
{
    if (!GuideSpline)
    {
        return FVector::ZeroVector;
    }

    // Get closest point info
    FVector ClosestPoint;
    float Distance;
    float InputKey = GetClosestPointOnSpline(Location, ClosestPoint, Distance);

    // Get tangent (direction along spline) at closest point
    FVector Tangent = GuideSpline->GetTangentAtSplineInputKey(InputKey, ESplineCoordinateSpace::World).GetSafeNormal();

    FVector FlowDirection = Tangent;

    // Optionally blend in pull towards the spline center
    if (bPullTowardsSpline && Distance > KINDA_SMALL_NUMBER)
    {
        FVector PullDirection = (ClosestPoint - Location).GetSafeNormal();
        FlowDirection = FMath::Lerp(Tangent, PullDirection, PullStrength).GetSafeNormal();
    }

    return FlowDirection;
}

float ACattleGuideSpline::GetClosestPointOnSpline(const FVector &WorldLocation, FVector &OutClosestPoint, float &OutDistance) const
{
    if (!GuideSpline)
    {
        OutClosestPoint = WorldLocation;
        OutDistance = 0.0f;
        return 0.0f;
    }

    // Find closest point on spline
    float InputKey = GuideSpline->FindInputKeyClosestToWorldLocation(WorldLocation);
    OutClosestPoint = GuideSpline->GetLocationAtSplineInputKey(InputKey, ESplineCoordinateSpace::World);
    OutDistance = FVector::Dist(WorldLocation, OutClosestPoint);

    return InputKey;
}

void ACattleGuideSpline::UpdateProximityCattle()
{
    if (!GetWorld())
    {
        return;
    }

    // Find all cattle actors in the world
    TArray<AActor *> CattleActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACattleAnimal::StaticClass(), CattleActors);

    TSet<TWeakObjectPtr<ACattleAnimal>> CurrentCattleInRange;

    for (AActor *Actor : CattleActors)
    {
        ACattleAnimal *Cattle = Cast<ACattleAnimal>(Actor);
        if (!Cattle)
        {
            continue;
        }

        // Check distance to spline
        FVector ClosestPoint;
        float Distance;
        GetClosestPointOnSpline(Cattle->GetActorLocation(), ClosestPoint, Distance);

        if (Distance <= InfluenceRadius)
        {
            CurrentCattleInRange.Add(Cattle);

            // Check if this is a new entry
            if (!CattleInProximity.Contains(Cattle))
            {
                HandleCattleEnterProximity(Cattle);
            }
        }
    }

    // Check for cattle that left proximity
    for (auto It = CattleInProximity.CreateIterator(); It; ++It)
    {
        if (!It->IsValid() || !CurrentCattleInRange.Contains(*It))
        {
            if (It->IsValid())
            {
                HandleCattleExitProximity(It->Get());
            }
            It.RemoveCurrent();
        }
    }

    // Update the proximity set
    CattleInProximity = CurrentCattleInRange;
}

void ACattleGuideSpline::HandleCattleEnterProximity(ACattleAnimal *Cattle)
{
    if (!Cattle)
    {
        return;
    }

    // Register this flow source with the cattle
    Cattle->AddActiveFlowSource(this);
    CattleInProximity.Add(Cattle);

    // Apply Gameplay Effect if configured
    if (ApplyEffectClass)
    {
        UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cattle);
        if (ASC)
        {
            FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
            EffectContext.AddSourceObject(this);

            FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ApplyEffectClass, 1.0f, EffectContext);
            if (SpecHandle.IsValid())
            {
                FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                ActiveEffectHandles.Add(Cattle, ActiveHandle);
            }
        }
    }
}

void ACattleGuideSpline::HandleCattleExitProximity(ACattleAnimal *Cattle)
{
    if (!Cattle)
    {
        return;
    }

    // Unregister this flow source from the cattle
    Cattle->RemoveActiveFlowSource(this);

    // Remove Gameplay Effect if it was applied
    if (ActiveEffectHandles.Contains(Cattle))
    {
        UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cattle);
        if (ASC)
        {
            ASC->RemoveActiveGameplayEffect(ActiveEffectHandles[Cattle], 1);
        }
        ActiveEffectHandles.Remove(Cattle);
    }
}

void ACattleGuideSpline::DrawDebugVisualization() const
{
    if (!GetWorld() || !GuideSpline)
    {
        return;
    }

    // Draw the spline itself
    const int32 NumSegments = GuideSpline->GetNumberOfSplinePoints() * 10;
    float SplineLength = GuideSpline->GetSplineLength();

    for (int32 i = 0; i < NumSegments; ++i)
    {
        float StartDist = (float)i / (float)NumSegments * SplineLength;
        float EndDist = (float)(i + 1) / (float)NumSegments * SplineLength;

        FVector StartPos = GuideSpline->GetLocationAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::World);
        FVector EndPos = GuideSpline->GetLocationAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::World);

        DrawDebugLine(GetWorld(), StartPos, EndPos, DebugColor, false, -1.0f, 0, 3.0f);
    }

    // Draw influence radius as a tube around the spline (sample points)
    const int32 NumRadiusSamples = 20;
    const int32 NumCircleSegments = 8;

    for (int32 i = 0; i <= NumRadiusSamples; ++i)
    {
        float Dist = (float)i / (float)NumRadiusSamples * SplineLength;
        FVector SplinePos = GuideSpline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
        FVector SplineTangent = GuideSpline->GetTangentAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World).GetSafeNormal();

        // Draw a circle perpendicular to the spline
        FVector Right = FVector::CrossProduct(SplineTangent, FVector::UpVector).GetSafeNormal();
        FVector Up = FVector::CrossProduct(Right, SplineTangent).GetSafeNormal();

        for (int32 j = 0; j < NumCircleSegments; ++j)
        {
            float Angle1 = (float)j / (float)NumCircleSegments * 2.0f * PI;
            float Angle2 = (float)(j + 1) / (float)NumCircleSegments * 2.0f * PI;

            FVector Offset1 = (Right * FMath::Cos(Angle1) + Up * FMath::Sin(Angle1)) * InfluenceRadius;
            FVector Offset2 = (Right * FMath::Cos(Angle2) + Up * FMath::Sin(Angle2)) * InfluenceRadius;

            FColor FadedColor = FColor(DebugColor.R / 2, DebugColor.G / 2, DebugColor.B / 2, 128);
            DrawDebugLine(GetWorld(), SplinePos + Offset1, SplinePos + Offset2, FadedColor, false, -1.0f, 0, 1.0f);
        }
    }

    // Draw flow arrows along the spline
    const int32 NumArrows = 10;
    for (int32 i = 0; i < NumArrows; ++i)
    {
        float Dist = (float)i / (float)NumArrows * SplineLength;
        FVector ArrowStart = GuideSpline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
        FVector Tangent = GuideSpline->GetTangentAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World).GetSafeNormal();
        FVector ArrowEnd = ArrowStart + Tangent * 150.0f;

        DrawDebugDirectionalArrow(GetWorld(), ArrowStart, ArrowEnd, 40.0f, DebugColor, false, -1.0f, 0, 3.0f);
    }

    // Draw cattle currently in proximity
    for (const TWeakObjectPtr<ACattleAnimal> &CattlePtr : CattleInProximity)
    {
        if (CattlePtr.IsValid())
        {
            DrawDebugSphere(GetWorld(), CattlePtr->GetActorLocation(), 50.0f, 8, FColor::Yellow, false, -1.0f, 0, 2.0f);
        }
    }
}
