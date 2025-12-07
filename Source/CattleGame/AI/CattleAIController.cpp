#include "CattleAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"

ACattleAIController::ACattleAIController()
{
}

void ACattleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ACattleAnimal* Cattle = Cast<ACattleAnimal>(InPawn);
	if (Cattle)
	{
		// GAS Initialization is usually handled in Character,
		// but if we needed to init it here we could.
		
		if (BehaviorTreeAsset)
		{
			RunBehaviorTree(BehaviorTreeAsset);
		}
	}
}

void ACattleAIController::OnUnPossess()
{
	Super::OnUnPossess();
}
