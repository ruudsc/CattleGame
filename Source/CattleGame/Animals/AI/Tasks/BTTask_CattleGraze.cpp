// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_CattleGraze.h"
#include "AIController.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Animals/CattleAnimalMovementComponent.h"

UBTTask_CattleGraze::UBTTask_CattleGraze()
{
    NodeName = "Cattle Graze";
    bNotifyTick = true;
}

uint16 UBTTask_CattleGraze::GetInstanceMemorySize() const
{
    return sizeof(FBTGrazeTaskMemory);
}

EBTNodeResult::Type UBTTask_CattleGraze::ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory)
{
    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }

    ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
    if (!Animal)
    {
        return EBTNodeResult::Failed;
    }

    // Set up task memory
    FBTGrazeTaskMemory *Memory = reinterpret_cast<FBTGrazeTaskMemory *>(NodeMemory);
    Memory->GrazeDuration = FMath::FRandRange(MinGrazeDuration, MaxGrazeDuration);
    Memory->ElapsedTime = 0.0f;

    // Set movement to grazing mode (slow/stopped)
    if (UCattleAnimalMovementComponent *Movement = Animal->GetAnimalMovement())
    {
        Movement->SetMovementMode_Grazing();
        Movement->StopMovementImmediately();
    }

    return EBTNodeResult::InProgress;
}

void UBTTask_CattleGraze::TickTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds)
{
    FBTGrazeTaskMemory *Memory = reinterpret_cast<FBTGrazeTaskMemory *>(NodeMemory);
    Memory->ElapsedTime += DeltaSeconds;

    // Check if graze duration is complete
    if (Memory->ElapsedTime >= Memory->GrazeDuration)
    {
        // Restore normal movement
        AAIController *AIController = OwnerComp.GetAIOwner();
        if (AIController)
        {
            ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
            if (Animal)
            {
                if (UCattleAnimalMovementComponent *Movement = Animal->GetAnimalMovement())
                {
                    Movement->SetMovementMode_Walking();
                }
            }
        }

        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // Random chance to trigger graze animation/sound
    if (FMath::FRand() < GrazeAnimationChance * DeltaSeconds)
    {
        // TODO: Trigger graze animation montage or sound
        // Animal->PlayGrazeAnimation();
    }
}
