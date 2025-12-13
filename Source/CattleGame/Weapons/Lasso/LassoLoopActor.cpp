#include "LassoLoopActor.h"
#include "Components/StaticMeshComponent.h"
#include "CattleGame/CattleGame.h"

ALassoLoopActor::ALassoLoopActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create loop mesh - assign actual mesh in Blueprint
	LoopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LoopMesh"));
	RootComponent = LoopMesh;
	LoopMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LoopMesh->SetCastShadow(false);
}

void ALassoLoopActor::BeginPlay()
{
	Super::BeginPlay();

	// Apply scale
	SetActorScale3D(FVector(LoopScale));

	if (!LoopMesh->GetStaticMesh())
	{
		UE_LOG(LogLasso, Warning, TEXT("LassoLoopActor: No mesh assigned to LoopMesh component - assign in Blueprint"));
	}
	else
	{
		UE_LOG(LogLasso, Log, TEXT("LassoLoopActor: Spawned with mesh %s at %s, scale %.2f"),
			   *GetNameSafe(LoopMesh->GetStaticMesh()), *GetActorLocation().ToString(), LoopScale);
	}
}
