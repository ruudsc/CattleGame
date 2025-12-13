// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_IsLassoed.h"
#include "AIController.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Weapons/Lasso/LassoableComponent.h"

UBTDecorator_IsLassoed::UBTDecorator_IsLassoed()
{
    NodeName = "Is Lassoed";
}

bool UBTDecorator_IsLassoed::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return false;
    }

    ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
    if (!Animal)
    {
        return false;
    }

    // Check if the animal has a LassoableComponent and is currently lassoed
    ULassoableComponent *LassoComp = Animal->FindComponentByClass<ULassoableComponent>();
    if (LassoComp)
    {
        return LassoComp->bIsLassoed;
    }

    return false;
}

FString UBTDecorator_IsLassoed::GetStaticDescription() const
{
    return TEXT("Returns true if cattle is currently captured by lasso");
}
