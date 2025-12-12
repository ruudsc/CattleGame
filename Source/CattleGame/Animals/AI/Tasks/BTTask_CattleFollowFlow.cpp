// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_CattleFollowFlow.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

UBTTask_CattleFollowFlow::UBTTask_CattleFollowFlow()
{
    NodeName = "Cattle Follow Flow";

    FlowDirectionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleFollowFlow, FlowDirectionKey));
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleFollowFlow, TargetLocationKey));
}

EBTNodeResult::Type UBTTask_CattleFollowFlow::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
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

    // Get flow direction
    FVector FlowDirection = BlackboardComp->GetValueAsVector(FlowDirectionKey.SelectedKeyName);
    FlowDirection.Z = 0.0f;

    if (FlowDirection.IsNearlyZero())
    {
        return EBTNodeResult::Failed;
    }

    FlowDirection.Normalize();

    // Calculate target location
    FVector TargetLocation = Pawn->GetActorLocation() + FlowDirection * FlowDistance;

    // Try to project to navigation
    UNavigationSystemV1 *NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys)
    {
        FNavLocation NavLocation;
        if (NavSys->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(500.0f, 500.0f, 500.0f)))
        {
            TargetLocation = NavLocation.Location;
        }
    }

    BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, TargetLocation);

    return EBTNodeResult::Succeeded;
}

FString UBTTask_CattleFollowFlow::GetStaticDescription() const
{
    return FString::Printf(TEXT("Follow flow direction (distance: %.0f)"), FlowDistance);
}
