#include "CattleFlowActorBase.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"

ACattleFlowActorBase::ACattleFlowActorBase()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false; // Only tick when debug is enabled

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootSceneComponent;
}

void ACattleFlowActorBase::BeginPlay()
{
    Super::BeginPlay();

    // Enable tick only if debug visualization is requested
    SetActorTickEnabled(bShowDebugFlow);
}

void ACattleFlowActorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ACattleFlowActorBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShowDebugFlow)
    {
        DrawDebugVisualization();
    }
}

// ===== ICattleFlowSource Interface Implementation =====

FVector ACattleFlowActorBase::GetFlowDirection_Implementation(const FVector &Location, float &OutWeight) const
{
    // Calculate distance for falloff
    FVector ClosestPoint = GetActorLocation(); // Override in derived classes for better precision
    float Distance = FVector::Dist(Location, ClosestPoint);

    // Calculate weight based on falloff
    OutWeight = CalculateFalloffWeight(Distance);

    if (OutWeight <= 0.0f)
    {
        return FVector::ZeroVector;
    }

    return CalculateFlowDirectionInternal(Location);
}

TSubclassOf<UGameplayEffect> ACattleFlowActorBase::GetApplyEffectClass_Implementation() const
{
    return ApplyEffectClass;
}

float ACattleFlowActorBase::GetInfluenceRadius_Implementation() const
{
    return InfluenceRadius;
}

UCurveFloat *ACattleFlowActorBase::GetFalloffCurve_Implementation() const
{
    return FalloffCurve;
}

int32 ACattleFlowActorBase::GetFlowPriority_Implementation() const
{
    return Priority;
}

bool ACattleFlowActorBase::IsProximityBased_Implementation() const
{
    // Default is overlap-based (volumes). Splines override to return true.
    return false;
}

AActor *ACattleFlowActorBase::GetFlowSourceActor_Implementation() const
{
    return const_cast<ACattleFlowActorBase *>(this);
}

// ===== Protected Methods =====

FVector ACattleFlowActorBase::CalculateFlowDirectionInternal(const FVector &Location) const
{
    // Base implementation returns zero. Override in derived classes.
    return FVector::ZeroVector;
}

float ACattleFlowActorBase::CalculateFalloffWeight(float Distance) const
{
    if (InfluenceRadius <= 0.0f)
    {
        // No influence radius set - always full weight (for volume-based sources)
        return 1.0f;
    }

    if (Distance >= InfluenceRadius)
    {
        return 0.0f;
    }

    // Normalize distance (0 = at center, 1 = at edge)
    float NormalizedDistance = Distance / InfluenceRadius;

    if (FalloffCurve)
    {
        // Use custom falloff curve
        return FMath::Clamp(FalloffCurve->GetFloatValue(NormalizedDistance), 0.0f, 1.0f);
    }
    else
    {
        // Default linear falloff (1 at center, 0 at edge)
        return 1.0f - NormalizedDistance;
    }
}

void ACattleFlowActorBase::DrawDebugVisualization() const
{
    if (!GetWorld())
    {
        return;
    }

    FVector Center = GetActorLocation();

    // Draw influence radius sphere
    if (InfluenceRadius > 0.0f)
    {
        DrawDebugSphere(GetWorld(), Center, InfluenceRadius, 24, DebugColor, false, -1.0f, 0, 2.0f);
    }

    // Draw flow direction arrow at center
    float DummyWeight;
    FVector FlowDir = GetFlowDirection_Implementation(Center, DummyWeight);
    if (!FlowDir.IsNearlyZero())
    {
        FVector ArrowEnd = Center + FlowDir * 200.0f;
        DrawDebugDirectionalArrow(GetWorld(), Center, ArrowEnd, 50.0f, DebugColor, false, -1.0f, 0, 3.0f);
    }

    // Draw some sample flow directions around the actor
    const int32 NumSamples = 8;
    const float SampleRadius = InfluenceRadius > 0.0f ? InfluenceRadius * 0.5f : 200.0f;

    for (int32 i = 0; i < NumSamples; ++i)
    {
        float Angle = (float)i / (float)NumSamples * 2.0f * PI;
        FVector SamplePos = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * SampleRadius;

        float Weight;
        FVector SampleFlow = GetFlowDirection_Implementation(SamplePos, Weight);

        if (!SampleFlow.IsNearlyZero() && Weight > 0.0f)
        {
            // Scale arrow by weight
            FVector ArrowEnd = SamplePos + SampleFlow * 100.0f * Weight;
            FColor WeightedColor = FColor(
                (uint8)(DebugColor.R * Weight),
                (uint8)(DebugColor.G * Weight),
                (uint8)(DebugColor.B * Weight));
            DrawDebugDirectionalArrow(GetWorld(), SamplePos, ArrowEnd, 25.0f, WeightedColor, false, -1.0f, 0, 2.0f);
        }
    }
}
