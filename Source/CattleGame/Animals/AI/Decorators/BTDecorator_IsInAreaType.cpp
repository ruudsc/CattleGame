// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_IsInAreaType.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"

UBTDecorator_IsInAreaType::UBTDecorator_IsInAreaType()
{
    NodeName = "Is In Area Type";

    CurrentAreaTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInAreaType, CurrentAreaTypeKey), StaticEnum<ECattleAreaType>());
}

bool UBTDecorator_IsInAreaType::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    ECattleAreaType CurrentAreaType = ECattleAreaType::None;

    if (bCheckDirectly)
    {
        AAIController *AIController = OwnerComp.GetAIOwner();
        if (AIController)
        {
            ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
            if (Animal)
            {
                FCattleAreaInfluence Influence = Animal->GetCurrentAreaInfluence();
                CurrentAreaType = Influence.AreaType;
            }
        }
    }
    else
    {
        UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
        if (BlackboardComp && CurrentAreaTypeKey.SelectedKeyName != NAME_None)
        {
            CurrentAreaType = static_cast<ECattleAreaType>(BlackboardComp->GetValueAsEnum(CurrentAreaTypeKey.SelectedKeyName));
        }
    }

    return CurrentAreaType == RequiredAreaType;
}

FString UBTDecorator_IsInAreaType::GetStaticDescription() const
{
    const UEnum *EnumPtr = StaticEnum<ECattleAreaType>();
    FString AreaTypeName = EnumPtr ? EnumPtr->GetNameStringByValue(static_cast<int64>(RequiredAreaType)) : TEXT("Unknown");

    if (bCheckDirectly)
    {
        return FString::Printf(TEXT("Is in %s area (direct check)"), *AreaTypeName);
    }
    return FString::Printf(TEXT("Is in %s area"), *AreaTypeName);
}
