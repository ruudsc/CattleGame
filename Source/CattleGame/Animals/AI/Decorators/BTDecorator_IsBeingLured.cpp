// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_IsBeingLured.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_IsBeingLured::UBTDecorator_IsBeingLured()
{
    NodeName = "Is Being Lured";

    IsBeingLuredKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsBeingLured, IsBeingLuredKey));
    FearLevelKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsBeingLured, FearLevelKey));
}

bool UBTDecorator_IsBeingLured::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    const UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }

    // Check if being lured
    const bool bIsLured = BlackboardComp->GetValueAsBool(IsBeingLuredKey.SelectedKeyName);
    if (!bIsLured)
    {
        return false;
    }

    // Optionally check fear level - only respond to lure if calm enough
    if (FearLevelKey.SelectedKeyName != NAME_None && MaxFearForLure < 1.0f)
    {
        const float FearLevel = BlackboardComp->GetValueAsFloat(FearLevelKey.SelectedKeyName);
        if (FearLevel > MaxFearForLure)
        {
            return false; // Too scared to be lured
        }
    }

    return true;
}

FString UBTDecorator_IsBeingLured::GetStaticDescription() const
{
    FString Description = FString::Printf(TEXT("Is being lured (key: %s)"), *IsBeingLuredKey.SelectedKeyName.ToString());
    if (MaxFearForLure < 1.0f)
    {
        Description += FString::Printf(TEXT("\nMax fear: %.0f%%"), MaxFearForLure * 100.0f);
    }
    return Description;
}
