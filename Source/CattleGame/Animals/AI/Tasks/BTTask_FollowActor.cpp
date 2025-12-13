// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_FollowActor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_FollowActor::UBTTask_FollowActor()
{
    NodeName = "Follow Actor";

    ActorToFollowKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FollowActor, ActorToFollowKey), AActor::StaticClass());
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FollowActor, TargetLocationKey));
}

EBTNodeResult::Type UBTTask_FollowActor::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
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

    // Get actor to follow
    AActor *ActorToFollow = Cast<AActor>(BlackboardComp->GetValueAsObject(ActorToFollowKey.SelectedKeyName));
    if (!ActorToFollow)
    {
        return EBTNodeResult::Failed;
    }

    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector TargetActorLocation = ActorToFollow->GetActorLocation();
    const float DistanceToTarget = FVector::Dist(PawnLocation, TargetActorLocation);

    // If already close enough, succeed without moving
    if (DistanceToTarget <= AcceptableRadius)
    {
        return EBTNodeResult::Succeeded;
    }

    // Calculate target location (move toward actor but stop at acceptable radius)
    FVector DirectionToTarget = (TargetActorLocation - PawnLocation).GetSafeNormal();
    FVector TargetLocation = TargetActorLocation - (DirectionToTarget * AcceptableRadius * 0.5f);

    // Set target location in blackboard
    BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, TargetLocation);

    // Optionally adjust movement speed
    if (ACharacter *Character = Cast<ACharacter>(Pawn))
    {
        if (UCharacterMovementComponent *MovementComp = Character->GetCharacterMovement())
        {
            // Note: Speed is controlled by the character's movement component
            // This task just sets the target - actual speed control could be done via
            // a gameplay effect or by modifying the movement component
        }
    }

    return EBTNodeResult::Succeeded;
}

FString UBTTask_FollowActor::GetStaticDescription() const
{
    return FString::Printf(TEXT("Follow %s (stop at %.0f units)"),
                           *ActorToFollowKey.SelectedKeyName.ToString(), AcceptableRadius);
}
