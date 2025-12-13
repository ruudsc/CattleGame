// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_FleeFromActor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

UBTTask_FleeFromActor::UBTTask_FleeFromActor()
{
    NodeName = "Flee From Actor";

    ActorToFleeFromKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FleeFromActor, ActorToFleeFromKey), AActor::StaticClass());
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FleeFromActor, TargetLocationKey));
}

EBTNodeResult::Type UBTTask_FleeFromActor::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
{
    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }

    APawn *Pawn = AIController->GetPawn();
    if (!Pawn)
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    // Get actor to flee from
    AActor *ActorToFleeFrom = Cast<AActor>(BlackboardComp->GetValueAsObject(ActorToFleeFromKey.SelectedKeyName));
    if (!ActorToFleeFrom)
    {
        return EBTNodeResult::Failed;
    }

    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector ThreatLocation = ActorToFleeFrom->GetActorLocation();

    // Calculate flee direction (away from threat)
    FVector FleeDirection = (PawnLocation - ThreatLocation).GetSafeNormal();
    FleeDirection.Z = 0.0f;

    if (FleeDirection.IsNearlyZero())
    {
        // If on top of threat, pick random direction
        const float RandomAngle = FMath::FRand() * 2.0f * PI;
        FleeDirection = FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f);
    }

    // Add random angle variation
    if (RandomAngleVariation > 0.0f)
    {
        const float AngleOffset = FMath::FRandRange(-RandomAngleVariation, RandomAngleVariation);
        FleeDirection = FleeDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);
    }

    // Calculate target location
    FVector TargetLocation = PawnLocation + (FleeDirection * FleeDistance);

    // Project to navigation if enabled
    if (bUseNavigation)
    {
        UNavigationSystemV1 *NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(500.0f, 500.0f, 500.0f)))
            {
                TargetLocation = NavLocation.Location;
            }
        }
    }

    // Set target location in blackboard
    BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, TargetLocation);

    return EBTNodeResult::Succeeded;
}

FString UBTTask_FleeFromActor::GetStaticDescription() const
{
    return FString::Printf(TEXT("Flee from %s (%.0f units)"),
                           *ActorToFleeFromKey.SelectedKeyName.ToString(), FleeDistance);
}
