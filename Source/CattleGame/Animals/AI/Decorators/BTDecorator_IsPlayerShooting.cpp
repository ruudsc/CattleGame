// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_IsPlayerShooting.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_IsPlayerShooting::UBTDecorator_IsPlayerShooting()
{
    NodeName = "Is Player Shooting";

    IsPlayerShootingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsPlayerShooting, IsPlayerShootingKey));
    FearLevelKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsPlayerShooting, FearLevelKey));
}

bool UBTDecorator_IsPlayerShooting::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    const UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }

    // Check if player is shooting
    const bool bIsShooting = BlackboardComp->GetValueAsBool(IsPlayerShootingKey.SelectedKeyName);
    if (!bIsShooting)
    {
        return false;
    }

    // Check fear level conditions
    if (FearLevelKey.SelectedKeyName != NAME_None)
    {
        const float FearLevel = BlackboardComp->GetValueAsFloat(FearLevelKey.SelectedKeyName);

        if (bRequireMinFear && FearLevel < MinFearLevel)
        {
            return false;
        }

        if (bRequireMaxFear && FearLevel > MaxFearLevel)
        {
            return false;
        }
    }

    return true;
}

FString UBTDecorator_IsPlayerShooting::GetStaticDescription() const
{
    FString Description = FString::Printf(TEXT("Player shooting nearby (key: %s)"), *IsPlayerShootingKey.SelectedKeyName.ToString());

    if (bRequireMinFear)
    {
        Description += FString::Printf(TEXT("\nMin fear: %.0f%%"), MinFearLevel * 100.0f);
    }
    if (bRequireMaxFear)
    {
        Description += FString::Printf(TEXT("\nMax fear: %.0f%%"), MaxFearLevel * 100.0f);
    }

    return Description;
}
