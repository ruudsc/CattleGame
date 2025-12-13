// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_FearLevel.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_FearLevel::UBTDecorator_FearLevel()
{
    NodeName = "Fear Level Check";

    FearLevelKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_FearLevel, FearLevelKey));
}

bool UBTDecorator_FearLevel::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp || FearLevelKey.SelectedKeyName == NAME_None)
    {
        return false;
    }

    const float FearLevel = BlackboardComp->GetValueAsFloat(FearLevelKey.SelectedKeyName);
    return FearLevel >= MinFearLevel && FearLevel <= MaxFearLevel;
}

FString UBTDecorator_FearLevel::GetStaticDescription() const
{
    return FString::Printf(TEXT("Fear level: %.0f%% - %.0f%%"), MinFearLevel * 100.0f, MaxFearLevel * 100.0f);
}
