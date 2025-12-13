// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAreaSubsystem.h"
#include "CattleAreaBase.h"
#include "CattleFlowGuide.h"
#include "DrawDebugHelpers.h"

void UCattleAreaSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    RegisteredAreas.Empty();
    RegisteredFlowGuides.Empty();
}

void UCattleAreaSubsystem::Deinitialize()
{
    RegisteredAreas.Empty();
    RegisteredFlowGuides.Empty();

    Super::Deinitialize();
}

bool UCattleAreaSubsystem::ShouldCreateSubsystem(UObject *Outer) const
{
    // Create in all worlds that might have gameplay
    if (UWorld *World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld() || World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Editor;
    }
    return false;
}

void UCattleAreaSubsystem::RegisterArea(ACattleAreaBase *Area)
{
    if (Area && !RegisteredAreas.Contains(Area))
    {
        RegisteredAreas.Add(Area);
    }
}

void UCattleAreaSubsystem::UnregisterArea(ACattleAreaBase *Area)
{
    RegisteredAreas.Remove(Area);
}

void UCattleAreaSubsystem::RegisterFlowGuide(ACattleFlowGuide *FlowGuide)
{
    if (FlowGuide && !RegisteredFlowGuides.Contains(FlowGuide))
    {
        RegisteredFlowGuides.Add(FlowGuide);
    }
}

void UCattleAreaSubsystem::UnregisterFlowGuide(ACattleFlowGuide *FlowGuide)
{
    RegisteredFlowGuides.Remove(FlowGuide);
}

TArray<FCattleAreaInfluence> UCattleAreaSubsystem::GetAreasAtLocation(const FVector &Location) const
{
    TArray<FCattleAreaInfluence> Results;

    for (const TWeakObjectPtr<ACattleAreaBase> &WeakArea : RegisteredAreas)
    {
        if (ACattleAreaBase *Area = WeakArea.Get())
        {
            FCattleAreaInfluence Influence = Area->GetInfluenceAtLocation(Location);
            if (Influence.IsValid())
            {
                Results.Add(Influence);
            }
        }
    }

    // Sort by priority (highest first)
    Results.Sort([](const FCattleAreaInfluence &A, const FCattleAreaInfluence &B)
                 { return A.Priority > B.Priority; });

    return Results;
}

FCattleAreaInfluence UCattleAreaSubsystem::GetPrimaryAreaAtLocation(const FVector &Location) const
{
    FCattleAreaInfluence HighestPriorityInfluence;
    int32 HighestPriority = -1;

    for (const TWeakObjectPtr<ACattleAreaBase> &WeakArea : RegisteredAreas)
    {
        if (ACattleAreaBase *Area = WeakArea.Get())
        {
            FCattleAreaInfluence Influence = Area->GetInfluenceAtLocation(Location);
            if (Influence.IsValid() && Influence.Priority > HighestPriority)
            {
                HighestPriority = Influence.Priority;
                HighestPriorityInfluence = Influence;
            }
        }
    }

    return HighestPriorityInfluence;
}

FVector UCattleAreaSubsystem::GetFlowDirectionAtLocation(const FVector &Location) const
{
    FVector AccumulatedFlow = FVector::ZeroVector;
    float TotalWeight = 0.0f;

    for (const TWeakObjectPtr<ACattleFlowGuide> &WeakFlowGuide : RegisteredFlowGuides)
    {
        if (ACattleFlowGuide *FlowGuide = WeakFlowGuide.Get())
        {
            float Weight = 0.0f;
            FVector FlowDir = FlowGuide->SampleFlowAtLocation(Location, Weight);

            if (Weight > 0.0f)
            {
                AccumulatedFlow += FlowDir * Weight;
                TotalWeight += Weight;
            }
        }
    }

    if (TotalWeight > 0.0f)
    {
        return (AccumulatedFlow / TotalWeight).GetSafeNormal();
    }

    return FVector::ZeroVector;
}

bool UCattleAreaSubsystem::IsLocationInAreaType(const FVector &Location, ECattleAreaType AreaType) const
{
    for (const TWeakObjectPtr<ACattleAreaBase> &WeakArea : RegisteredAreas)
    {
        if (ACattleAreaBase *Area = WeakArea.Get())
        {
            if (Area->GetAreaType() == AreaType && Area->IsLocationInArea(Location))
            {
                return true;
            }
        }
    }

    return false;
}

TArray<ACattleAreaBase *> UCattleAreaSubsystem::GetAreasOfType(ECattleAreaType AreaType) const
{
    TArray<ACattleAreaBase *> Results;

    for (const TWeakObjectPtr<ACattleAreaBase> &WeakArea : RegisteredAreas)
    {
        if (ACattleAreaBase *Area = WeakArea.Get())
        {
            if (Area->GetAreaType() == AreaType)
            {
                Results.Add(Area);
            }
        }
    }

    return Results;
}

void UCattleAreaSubsystem::DrawDebugAreas(float Duration) const
{
    UWorld *World = GetWorld();
    if (!World)
        return;

    for (const TWeakObjectPtr<ACattleAreaBase> &WeakArea : RegisteredAreas)
    {
        if (ACattleAreaBase *Area = WeakArea.Get())
        {
            Area->DrawDebugArea(Duration);
        }
    }

    for (const TWeakObjectPtr<ACattleFlowGuide> &WeakFlowGuide : RegisteredFlowGuides)
    {
        if (ACattleFlowGuide *FlowGuide = WeakFlowGuide.Get())
        {
            FlowGuide->DrawDebugFlow(Duration);
        }
    }
}

void UCattleAreaSubsystem::CleanupInvalidReferences()
{
    RegisteredAreas.RemoveAll([](const TWeakObjectPtr<ACattleAreaBase> &WeakArea)
                              { return !WeakArea.IsValid(); });

    RegisteredFlowGuides.RemoveAll([](const TWeakObjectPtr<ACattleFlowGuide> &WeakFlowGuide)
                                   { return !WeakFlowGuide.IsValid(); });
}
