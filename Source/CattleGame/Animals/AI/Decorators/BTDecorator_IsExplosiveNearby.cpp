// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_IsExplosiveNearby.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_IsExplosiveNearby::UBTDecorator_IsExplosiveNearby()
{
    NodeName = "Is Explosive Nearby";

    NearbyExplosiveKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsExplosiveNearby, NearbyExplosiveKey), AActor::StaticClass());
}

bool UBTDecorator_IsExplosiveNearby::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    const UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }

    // Check if there's an explosive set in the blackboard
    UObject *ExplosiveObject = BlackboardComp->GetValueAsObject(NearbyExplosiveKey.SelectedKeyName);
    return ExplosiveObject != nullptr;
}

FString UBTDecorator_IsExplosiveNearby::GetStaticDescription() const
{
    return FString::Printf(TEXT("Explosive nearby: %s"), *NearbyExplosiveKey.SelectedKeyName.ToString());
}
