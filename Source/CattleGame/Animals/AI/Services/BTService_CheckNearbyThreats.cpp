// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTService_CheckNearbyThreats.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UBTService_CheckNearbyThreats::UBTService_CheckNearbyThreats()
{
    NodeName = "Check Nearby Threats";
    Interval = 0.25f;
    RandomDeviation = 0.05f;

    NearestThreatKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_CheckNearbyThreats, NearestThreatKey), AActor::StaticClass());
    ThreatDistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_CheckNearbyThreats, ThreatDistanceKey));
}

void UBTService_CheckNearbyThreats::TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }

    ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
    if (!Animal)
    {
        return;
    }

    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    // Find nearest threat
    AActor *NearestThreat = FindNearestThreat(AIController);
    float ThreatDistance = ThreatDetectionRadius + 1.0f;

    if (NearestThreat)
    {
        ThreatDistance = FVector::Dist(Animal->GetActorLocation(), NearestThreat->GetActorLocation());

        // Add fear based on proximity
        if (ThreatDistance < FearStartDistance)
        {
            const float ProximityFactor = 1.0f - (ThreatDistance / FearStartDistance);
            const float FearToAdd = MaxFearPerSecond * ProximityFactor * DeltaSeconds;
            Animal->AddFear(FearToAdd);
        }
    }

    // Update blackboard
    if (NearestThreatKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsObject(NearestThreatKey.SelectedKeyName, NearestThreat);
    }

    if (ThreatDistanceKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsFloat(ThreatDistanceKey.SelectedKeyName, ThreatDistance);
    }
}

AActor *UBTService_CheckNearbyThreats::FindNearestThreat(AAIController *AIController) const
{
    APawn *Pawn = AIController->GetPawn();
    if (!Pawn)
    {
        return nullptr;
    }

    UWorld *World = Pawn->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector PawnLocation = Pawn->GetActorLocation();
    AActor *NearestThreat = nullptr;
    float NearestDistance = ThreatDetectionRadius;

    // Check players if enabled
    if (bPlayersAreThreat)
    {
        TArray<AActor *> PlayerPawns;
        UGameplayStatics::GetAllActorsOfClass(World, ACharacter::StaticClass(), PlayerPawns);

        for (AActor *PlayerPawn : PlayerPawns)
        {
            // Skip if it's another cattle animal
            if (PlayerPawn->IsA(ACattleAnimal::StaticClass()))
            {
                continue;
            }

            // Skip if it's the same pawn
            if (PlayerPawn == Pawn)
            {
                continue;
            }

            const float Distance = FVector::Dist(PawnLocation, PlayerPawn->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestThreat = PlayerPawn;
            }
        }
    }

    // Check configured threat classes
    for (const TSubclassOf<AActor> &ThreatClass : ThreatClasses)
    {
        if (!ThreatClass)
        {
            continue;
        }

        TArray<AActor *> ThreatActors;
        UGameplayStatics::GetAllActorsOfClass(World, ThreatClass, ThreatActors);

        for (AActor *ThreatActor : ThreatActors)
        {
            if (ThreatActor == Pawn)
            {
                continue;
            }

            const float Distance = FVector::Dist(PawnLocation, ThreatActor->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestThreat = ThreatActor;
            }
        }
    }

    return NearestThreat;
}

FString UBTService_CheckNearbyThreats::GetStaticDescription() const
{
    return FString::Printf(TEXT("Scan for threats within %.0f units"), ThreatDetectionRadius);
}
