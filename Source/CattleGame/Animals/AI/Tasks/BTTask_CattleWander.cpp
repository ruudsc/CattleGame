// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_CattleWander.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogCattleWander, Log, All);

UBTTask_CattleWander::UBTTask_CattleWander()
{
    NodeName = "Cattle Wander";

    // Set up blackboard key filters
    HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleWander, HomeLocationKey));
    WanderRadiusKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleWander, WanderRadiusKey));
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_CattleWander, TargetLocationKey));
}

EBTNodeResult::Type UBTTask_CattleWander::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
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
        UE_LOG(LogCattleWander, Error, TEXT("[CattleWander] No BlackboardComponent"));
        return EBTNodeResult::Failed;
    }

    // Get wander parameters
    const FVector HomeLocation = BlackboardComp->GetValueAsVector(HomeLocationKey.SelectedKeyName);
    const float WanderRadius = BlackboardComp->GetValueAsFloat(WanderRadiusKey.SelectedKeyName);
    const FVector CurrentLocation = Pawn->GetActorLocation();

    UE_LOG(LogCattleWander, Log, TEXT("[CattleWander] HomeLocation: %s, WanderRadius: %.1f, CurrentLocation: %s"),
           *HomeLocation.ToString(), WanderRadius, *CurrentLocation.ToString());

    FVector TargetLocation = FVector::ZeroVector;
    bool bFoundLocation = false;

    // Try to find a valid wander location
    for (int32 Attempt = 0; Attempt < 10; ++Attempt)
    {
        // Random point within wander radius of home
        const float RandomAngle = FMath::FRand() * 2.0f * PI;
        const float RandomRadius = FMath::FRand() * WanderRadius;

        FVector TestLocation = HomeLocation + FVector(
                                                  FMath::Cos(RandomAngle) * RandomRadius,
                                                  FMath::Sin(RandomAngle) * RandomRadius,
                                                  0.0f);

        // Check minimum distance from current location
        const float DistanceToCurrent = FVector::Dist2D(TestLocation, CurrentLocation);
        if (DistanceToCurrent < MinWanderDistance)
        {
            UE_LOG(LogCattleWander, Verbose, TEXT("[CattleWander] Attempt %d: Too close (%.1f < %.1f)"),
                   Attempt, DistanceToCurrent, MinWanderDistance);
            continue;
        }

        if (bUseNavigation)
        {
            // Project to navigation
            UNavigationSystemV1 *NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
            if (NavSys)
            {
                FNavLocation NavLocation;
                if (NavSys->ProjectPointToNavigation(TestLocation, NavLocation, FVector(500.0f, 500.0f, 500.0f)))
                {
                    TargetLocation = NavLocation.Location;
                    bFoundLocation = true;
                    UE_LOG(LogCattleWander, Log, TEXT("[CattleWander] Found location at attempt %d: %s"),
                           Attempt, *TargetLocation.ToString());
                    break;
                }
                else
                {
                    UE_LOG(LogCattleWander, Verbose, TEXT("[CattleWander] Attempt %d: Nav projection failed for %s"),
                           Attempt, *TestLocation.ToString());
                }
            }
            else
            {
                UE_LOG(LogCattleWander, Warning, TEXT("[CattleWander] No NavigationSystem found"));
            }
        }
        else
        {
            TargetLocation = TestLocation;
            bFoundLocation = true;
            break;
        }
    }

    if (bFoundLocation)
    {
        BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, TargetLocation);
        UE_LOG(LogCattleWander, Log, TEXT("[CattleWander] SUCCESS - Set target to: %s"), *TargetLocation.ToString());
        return EBTNodeResult::Succeeded;
    }

    UE_LOG(LogCattleWander, Warning, TEXT("[CattleWander] FAILED - Could not find valid wander location after 10 attempts"));
    return EBTNodeResult::Failed;
}

FString UBTTask_CattleWander::GetStaticDescription() const
{
    return FString::Printf(TEXT("Wander within radius (min dist: %.0f)"), MinWanderDistance);
}
