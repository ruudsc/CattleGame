// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_HasNearbyThreat.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HasNearbyThreat::UBTDecorator_HasNearbyThreat()
{
    NodeName = "Has Nearby Threat";

    NearestThreatKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasNearbyThreat, NearestThreatKey), AActor::StaticClass());
    ThreatDistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasNearbyThreat, ThreatDistanceKey));
}

bool UBTDecorator_HasNearbyThreat::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }

    // Check if we have a threat actor
    if (NearestThreatKey.SelectedKeyName != NAME_None)
    {
        UObject *ThreatObject = BlackboardComp->GetValueAsObject(NearestThreatKey.SelectedKeyName);
        if (!ThreatObject)
        {
            return false;
        }
    }

    // Check distance if key is set
    if (ThreatDistanceKey.SelectedKeyName != NAME_None)
    {
        const float Distance = BlackboardComp->GetValueAsFloat(ThreatDistanceKey.SelectedKeyName);
        return Distance <= MaxThreatDistance;
    }

    // If we have a threat but no distance key, return true
    return true;
}

FString UBTDecorator_HasNearbyThreat::GetStaticDescription() const
{
    return FString::Printf(TEXT("Has threat within %.0f units"), MaxThreatDistance);
}
