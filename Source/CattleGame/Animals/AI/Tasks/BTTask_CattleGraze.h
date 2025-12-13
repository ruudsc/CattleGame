// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CattleGraze.generated.h"

/**
 * UBTTask_CattleGraze
 *
 * Task that makes the cattle stop and graze for a random duration.
 * Simulates eating behavior with occasional head movements.
 */
UCLASS()
class CATTLEGAME_API UBTTask_CattleGraze : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CattleGraze();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
    virtual uint16 GetInstanceMemorySize() const override;

protected:
    virtual void TickTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds) override;

    /** Minimum graze duration in seconds */
    UPROPERTY(EditAnywhere, Category = "Graze", meta = (ClampMin = "1.0"))
    float MinGrazeDuration = 3.0f;

    /** Maximum graze duration in seconds */
    UPROPERTY(EditAnywhere, Category = "Graze", meta = (ClampMin = "1.0"))
    float MaxGrazeDuration = 8.0f;

    /** Chance to play a graze animation/sound per second */
    UPROPERTY(EditAnywhere, Category = "Graze", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GrazeAnimationChance = 0.3f;
};

/** Memory struct for graze task */
struct FBTGrazeTaskMemory
{
    float GrazeDuration;
    float ElapsedTime;
};
