// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAIController.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Animals/Areas/CattleAreaSubsystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCattleAI, Log, All);

// Blackboard Key Names
const FName ACattleAIController::KEY_TargetLocation = FName("TargetLocation");
const FName ACattleAIController::KEY_TargetActor = FName("TargetActor");
const FName ACattleAIController::KEY_CurrentAreaType = FName("CurrentAreaType");
const FName ACattleAIController::KEY_FearLevel = FName("FearLevel");
const FName ACattleAIController::KEY_IsPanicked = FName("IsPanicked");
const FName ACattleAIController::KEY_FlowDirection = FName("FlowDirection");
const FName ACattleAIController::KEY_HomeLocation = FName("HomeLocation");
const FName ACattleAIController::KEY_WanderRadius = FName("WanderRadius");

ACattleAIController::ACattleAIController()
{
    BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

    // Set this as the brains component
    BrainComponent = BehaviorTreeComp;
}

void ACattleAIController::BeginPlay()
{
    Super::BeginPlay();
}

void ACattleAIController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);

    UE_LOG(LogCattleAI, Log, TEXT("[CattleAIController] OnPossess: %s"), *InPawn->GetName());

    if (BehaviorTree && BehaviorTree->BlackboardAsset)
    {
        UE_LOG(LogCattleAI, Log, TEXT("[CattleAIController] Initializing BehaviorTree: %s with Blackboard: %s"),
               *BehaviorTree->GetName(), *BehaviorTree->BlackboardAsset->GetName());
        BlackboardComp->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
        InitializeCattleBlackboard();
        BehaviorTreeComp->StartTree(*BehaviorTree);
        UE_LOG(LogCattleAI, Log, TEXT("[CattleAIController] BehaviorTree started successfully"));
    }
    else
    {
        UE_LOG(LogCattleAI, Error, TEXT("[CattleAIController] No BehaviorTree or BlackboardAsset assigned! BehaviorTree=%s"),
               BehaviorTree ? *BehaviorTree->GetName() : TEXT("NULL"));
    }
}

void ACattleAIController::OnUnPossess()
{
    BehaviorTreeComp->StopTree();
    Super::OnUnPossess();
}

ACattleAnimal *ACattleAIController::GetCattleAnimal() const
{
    return Cast<ACattleAnimal>(GetPawn());
}

void ACattleAIController::UpdateFearBlackboard()
{
    if (!BlackboardComp)
    {
        return;
    }

    ACattleAnimal *Animal = GetCattleAnimal();
    if (!Animal)
    {
        return;
    }

    BlackboardComp->SetValueAsFloat(KEY_FearLevel, Animal->GetFearPercent());
    BlackboardComp->SetValueAsBool(KEY_IsPanicked, Animal->IsPanicked());
}

void ACattleAIController::UpdateAreaBlackboard()
{
    if (!BlackboardComp)
    {
        return;
    }

    ACattleAnimal *Animal = GetCattleAnimal();
    if (!Animal)
    {
        return;
    }

    FCattleAreaInfluence Influence = Animal->GetCurrentAreaInfluence();
    BlackboardComp->SetValueAsEnum(KEY_CurrentAreaType, static_cast<uint8>(Influence.AreaType));
    BlackboardComp->SetValueAsVector(KEY_FlowDirection, Influence.InfluenceDirection);
}

void ACattleAIController::SetTargetLocation(const FVector &Location)
{
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsVector(KEY_TargetLocation, Location);
    }
}

void ACattleAIController::SetTargetActor(AActor *Actor)
{
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsObject(KEY_TargetActor, Actor);
    }
}

void ACattleAIController::SetHomeLocation(const FVector &Location)
{
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsVector(KEY_HomeLocation, Location);
    }
}

void ACattleAIController::InitializeCattleBlackboard()
{
    if (!BlackboardComp || !GetPawn())
    {
        UE_LOG(LogCattleAI, Error, TEXT("[CattleAIController] InitializeCattleBlackboard failed - BlackboardComp=%s, Pawn=%s"),
               BlackboardComp ? TEXT("Valid") : TEXT("NULL"), GetPawn() ? TEXT("Valid") : TEXT("NULL"));
        return;
    }

    // Set initial home location to spawn position
    const FVector HomeLocation = GetPawn()->GetActorLocation();
    BlackboardComp->SetValueAsVector(KEY_HomeLocation, HomeLocation);
    BlackboardComp->SetValueAsFloat(KEY_WanderRadius, DefaultWanderRadius);
    BlackboardComp->SetValueAsFloat(KEY_FearLevel, 0.0f);
    BlackboardComp->SetValueAsBool(KEY_IsPanicked, false);
    BlackboardComp->SetValueAsEnum(KEY_CurrentAreaType, static_cast<uint8>(ECattleAreaType::None));

    UE_LOG(LogCattleAI, Log, TEXT("[CattleAIController] Initialized Blackboard - HomeLocation: %s, WanderRadius: %.1f"),
           *HomeLocation.ToString(), DefaultWanderRadius);
}
