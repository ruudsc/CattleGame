#include "BTTask_FindGrazingPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "CattleGame/AI/CattleAIController.h"
#include "NavigationSystem.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Volumes/Components/CattleGrazeLogic.h"
#include "CattleGame/Volumes/CattleVolume.h"

UBTTask_FindGrazingPoint::UBTTask_FindGrazingPoint()
{
	NodeName = "Find Grazing Point";
}

EBTNodeResult::Type UBTTask_FindGrazingPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ACattleAIController* MyController = Cast<ACattleAIController>(OwnerComp.GetAIOwner());
	if (!MyController) return EBTNodeResult::Failed;

	ACattleAnimal* MyPawn = Cast<ACattleAnimal>(MyController->GetPawn());
	if (!MyPawn) return EBTNodeResult::Failed;

	// 1. Get Active Graze Logic
	// Accessing the Animal's volume list. Unfortunately it's protected.
    // Need to expose an accessor or friend class. For now assuming accessor.
    // Let's add GetActiveVolumeLogics() to CattleAnimal.
	
	UCattleGrazeLogic* GrazeLogic = nullptr;
    // Iterate active logics (Need to implement access in Animal first, will add TODO)
    // For now, let's just create a getter in Animal.
    TArray<UCattleVolumeLogicComponent*> ActiveLogics = MyPawn->GetActiveVolumeLogics(); 
    
    for (auto* Logic : ActiveLogics)
    {
        if (UCattleGrazeLogic* GL = Cast<UCattleGrazeLogic>(Logic))
        {
            GrazeLogic = GL;
            break;
        }
    }

	FVector Origin = MyPawn->GetActorLocation();
    FVector ResultLocation = Origin;
    bool bFound = false;

	if (GrazeLogic)
    {
        // Pick random point in volume bounds
        if (ACattleVolume* Vol = GrazeLogic->GetOwningVolume())
        {
            FBox Bounds = Vol->GetComponentsBoundingBox();
            Origin = FMath::RandPointInBox(Bounds);
            // Project to NavMesh
            bFound = true;
        }
    }

    // Nav System
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys)
    {
        FNavLocation Loc;
        if (NavSys->GetRandomReachablePointInRadius(Origin, bFound ? 100.0f : FallbackRadius, Loc))
        {
            OwnerComp.GetBlackboardComponent()->SetValueAsVector(TargetLocationKey.SelectedKeyName, Loc.Location);
            return EBTNodeResult::Succeeded;
        }
    }

	return EBTNodeResult::Failed;
}
