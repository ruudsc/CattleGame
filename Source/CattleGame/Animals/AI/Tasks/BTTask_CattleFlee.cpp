// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_CattleFlee.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Animals/CattleAnimalMovementComponent.h"
#include "NavigationSystem.h"

UBTTask_CattleFlee::UBTTask_CattleFlee()
{
    NodeName = "Cattle Flee";

    FlowDirectionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleFlee, FlowDirectionKey));
    ThreatActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleFlee, ThreatActorKey), AActor::StaticClass());
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleFlee, TargetLocationKey));
}

EBTNodeResult::Type UBTTask_CattleFlee::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
{
    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }

    ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
    if (!Animal)
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    // Set movement to panic mode
    if (UCattleAnimalMovementComponent *Movement = Animal->GetAnimalMovement())
    {
        Movement->SetMovementMode_Panic();
    }

    FVector FleeDirection = FVector::ZeroVector;

    // First check for threat actor
    if (ThreatActorKey.SelectedKeyName != NAME_None)
    {
        AActor *ThreatActor = Cast<AActor>(BlackboardComp->GetValueAsObject(ThreatActorKey.SelectedKeyName));
        if (ThreatActor)
        {
            // Flee away from threat
            FleeDirection = Animal->GetActorLocation() - ThreatActor->GetActorLocation();
            FleeDirection.Z = 0.0f;
            FleeDirection.Normalize();
        }
    }

    // If no threat actor, use flow direction from area
    if (FleeDirection.IsNearlyZero() && FlowDirectionKey.SelectedKeyName != NAME_None)
    {
        FleeDirection = BlackboardComp->GetValueAsVector(FlowDirectionKey.SelectedKeyName);
        FleeDirection.Z = 0.0f;
    }

    // If still no direction, flee in random direction
    if (FleeDirection.IsNearlyZero())
    {
        const float RandomAngle = FMath::FRand() * 2.0f * PI;
        FleeDirection = FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f);
    }

    // Add random variation
    if (RandomAngleVariation > 0.0f)
    {
        const float AngleOffset = FMath::FRandRange(-RandomAngleVariation, RandomAngleVariation);
        FleeDirection = FleeDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);
    }

    // Calculate target location
    FVector TargetLocation = Animal->GetActorLocation() + FleeDirection * FleeDistance;

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

FString UBTTask_CattleFlee::GetStaticDescription() const
{
    return FString::Printf(TEXT("Flee (distance: %.0f, variation: %.0f°)"), FleeDistance, RandomAngleVariation);
}
