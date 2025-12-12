// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_IsCattlePanicked.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"

UBTDecorator_IsCattlePanicked::UBTDecorator_IsCattlePanicked()
{
    NodeName = "Is Cattle Panicked";

    IsPanickedKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsCattlePanicked, IsPanickedKey));
}

bool UBTDecorator_IsCattlePanicked::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    if (bCheckDirectly)
    {
        AAIController *AIController = OwnerComp.GetAIOwner();
        if (AIController)
        {
            ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
            if (Animal)
            {
                return Animal->IsPanicked();
            }
        }
        return false;
    }
    else
    {
        UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
        if (BlackboardComp && IsPanickedKey.SelectedKeyName != NAME_None)
        {
            return BlackboardComp->GetValueAsBool(IsPanickedKey.SelectedKeyName);
        }
        return false;
    }
}

FString UBTDecorator_IsCattlePanicked::GetStaticDescription() const
{
    if (bCheckDirectly)
    {
        return TEXT("Is Panicked (direct check)");
    }
    return FString::Printf(TEXT("Is Panicked: %s"), *IsPanickedKey.SelectedKeyName.ToString());
}
