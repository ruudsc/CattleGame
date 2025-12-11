#include "CattleFlowSubsystem.h"

void UCattleFlowSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);
}

void UCattleFlowSubsystem::Deinitialize()
{
    RegisteredProximitySources.Empty();
    Super::Deinitialize();
}

bool UCattleFlowSubsystem::ShouldCreateSubsystem(UObject *Outer) const
{
    // Create for all game worlds
    return true;
}

void UCattleFlowSubsystem::RegisterProximitySource(TScriptInterface<ICattleFlowSource> Source)
{
    if (Source.GetInterface() && !RegisteredProximitySources.Contains(Source))
    {
        RegisteredProximitySources.Add(Source);
    }
}

void UCattleFlowSubsystem::UnregisterProximitySource(TScriptInterface<ICattleFlowSource> Source)
{
    if (Source.GetInterface())
    {
        RegisteredProximitySources.Remove(Source);
    }
}

TArray<TScriptInterface<ICattleFlowSource>> UCattleFlowSubsystem::QueryNearbyProximitySources(const FVector &Location, float QueryRadius) const
{
    TArray<TScriptInterface<ICattleFlowSource>> Results;

    for (const TScriptInterface<ICattleFlowSource> &Source : RegisteredProximitySources)
    {
        if (!Source.GetInterface())
        {
            continue;
        }

        AActor *SourceActor = ICattleFlowSource::Execute_GetFlowSourceActor(Source.GetObject());
        if (!SourceActor)
        {
            continue;
        }

        // Get the influence radius of the source
        float SourceRadius = ICattleFlowSource::Execute_GetInfluenceRadius(Source.GetObject());

        // Use the larger of query radius or source radius for the check
        float EffectiveRadius = FMath::Max(QueryRadius, SourceRadius);

        // Simple distance check to actor location
        // For splines, we should ideally check distance to closest point on spline
        // but for now actor location works as a broad-phase filter
        float DistanceSq = FVector::DistSquared(Location, SourceActor->GetActorLocation());

        // Add some buffer for spline extent
        float CheckRadius = EffectiveRadius + 5000.0f; // Extra buffer for spline length

        if (DistanceSq <= FMath::Square(CheckRadius))
        {
            Results.Add(Source);
        }
    }

    return Results;
}
