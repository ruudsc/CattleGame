// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_LookAtActor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_LookAtActor::UBTTask_LookAtActor()
{
    NodeName = "Look At Actor";

    ActorToLookAtKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_LookAtActor, ActorToLookAtKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_LookAtActor::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
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

    // Get actor to look at
    AActor *ActorToLookAt = Cast<AActor>(BlackboardComp->GetValueAsObject(ActorToLookAtKey.SelectedKeyName));
    if (!ActorToLookAt)
    {
        return EBTNodeResult::Failed;
    }

    // Calculate direction to look
    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector TargetLocation = ActorToLookAt->GetActorLocation();
    FVector DirectionToTarget = (TargetLocation - PawnLocation).GetSafeNormal();
    DirectionToTarget.Z = 0.0f; // Keep horizontal

    if (DirectionToTarget.IsNearlyZero())
    {
        return EBTNodeResult::Succeeded;
    }

    // Set focus on the actor (AAIController will handle smooth rotation)
    AIController->SetFocalPoint(TargetLocation, EAIFocusPriority::Gameplay);

    return EBTNodeResult::Succeeded;
}

FString UBTTask_LookAtActor::GetStaticDescription() const
{
    return FString::Printf(TEXT("Look at %s"), *ActorToLookAtKey.SelectedKeyName.ToString());
}
