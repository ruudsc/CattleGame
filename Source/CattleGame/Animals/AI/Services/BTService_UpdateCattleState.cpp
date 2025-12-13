// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTService_UpdateCattleState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Animals/Areas/CattleAreaSubsystem.h"

UBTService_UpdateCattleState::UBTService_UpdateCattleState()
{
    NodeName = "Update Cattle State";
    Interval = 0.1f;
    RandomDeviation = 0.02f;

    // Set up blackboard key filters
    FearLevelKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCattleState, FearLevelKey));
    IsPanickedKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCattleState, IsPanickedKey));
    CurrentAreaTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCattleState, CurrentAreaTypeKey), StaticEnum<ECattleAreaType>());
    FlowDirectionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCattleState, FlowDirectionKey));
}

void UBTService_UpdateCattleState::TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds)
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

    // Update fear state
    if (FearLevelKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsFloat(FearLevelKey.SelectedKeyName, Animal->GetFearPercent());
    }

    if (IsPanickedKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsBool(IsPanickedKey.SelectedKeyName, Animal->IsPanicked());
    }

    // Update area influences
    if (bUpdateAreaInfluences)
    {
        Animal->UpdateAreaInfluences();
    }

    // Get current area influence
    FCattleAreaInfluence Influence = Animal->GetCurrentAreaInfluence();

    if (CurrentAreaTypeKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsEnum(CurrentAreaTypeKey.SelectedKeyName, static_cast<uint8>(Influence.AreaType));
    }

    if (FlowDirectionKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsVector(FlowDirectionKey.SelectedKeyName, Influence.InfluenceDirection);
    }
}

FString UBTService_UpdateCattleState::GetStaticDescription() const
{
    return FString::Printf(TEXT("Updates fear, panic, and area state every %.2fs"), Interval);
}
