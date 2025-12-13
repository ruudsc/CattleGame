// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTDecorator_HasFlowDirection.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HasFlowDirection::UBTDecorator_HasFlowDirection()
{
    NodeName = "Has Flow Direction";

    FlowDirectionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasFlowDirection, FlowDirectionKey));
}

bool UBTDecorator_HasFlowDirection::CalculateRawConditionValue(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) const
{
    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp || FlowDirectionKey.SelectedKeyName == NAME_None)
    {
        return false;
    }

    const FVector FlowDirection = BlackboardComp->GetValueAsVector(FlowDirectionKey.SelectedKeyName);
    return FlowDirection.Size() >= MinimumMagnitude;
}

FString UBTDecorator_HasFlowDirection::GetStaticDescription() const
{
    return FString::Printf(TEXT("Has flow direction (min: %.2f)"), MinimumMagnitude);
}
