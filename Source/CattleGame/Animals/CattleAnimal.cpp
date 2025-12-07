// Fill out your copyright notice in the Description page of Project Settings.


#include "CattleAnimal.h"
#include "CattleGame/Weapons/Lasso/LassoableComponent.h"
#include "CattleGame/AbilitySystem/CattleAbilitySystemComponent.h"
#include "CattleGame/AbilitySystem/CattleAttributeSet.h"
#include "CattleGame/Volumes/Components/CattleVolumeLogicComponent.h"
#include "CattleGame/AI/CattleAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ACattleAnimal::ACattleAnimal()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create lassoable component for lasso target support
	LassoableComponent = CreateDefaultSubobject<ULassoableComponent>(TEXT("LassoableComponent"));
	LassoableComponent->AttachSocketName = FName("pelvis"); // Default socket for cattle

	// AI
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ACattleAIController::StaticClass();

	// Ability System
	AbilitySystemComponent = CreateDefaultSubobject<UCattleAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UCattleAttributeSet>(TEXT("AttributeSet"));

}

// Called when the game starts or when spawned
void ACattleAnimal::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

// Called every frame
void ACattleAnimal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Poll Volumes for flow
	if (ActiveVolumeLogics.Num() > 0)
	{
		FVector FlowDir = CalculateVolumeFlowDirection();
		if (!FlowDir.IsNearlyZero())
		{
			AddMovementInput(FlowDir, 1.0f);
		}
	}
}

UAbilitySystemComponent* ACattleAnimal::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ACattleAnimal::AddActiveVolumeLogic(UCattleVolumeLogicComponent* LogicComp)
{
	if (LogicComp && !ActiveVolumeLogics.Contains(LogicComp))
	{
		ActiveVolumeLogics.Add(LogicComp);
	}
}

void ACattleAnimal::RemoveActiveVolumeLogic(UCattleVolumeLogicComponent* LogicComp)
{
	if (LogicComp)
	{
		ActiveVolumeLogics.Remove(LogicComp);
	}
}

TArray<UCattleVolumeLogicComponent*> ACattleAnimal::GetActiveVolumeLogics() const
{
	TArray<UCattleVolumeLogicComponent*> Result;
	for (auto& Comp : ActiveVolumeLogics)
	{
		if (Comp) Result.Add(Comp);
	}
	return Result;
}

FVector ACattleAnimal::CalculateVolumeFlowDirection() const
{
	FVector TotalFlow = FVector::ZeroVector;
	FVector MyLoc = GetActorLocation();

	for (UCattleVolumeLogicComponent* Logic : ActiveVolumeLogics)
	{
		if (Logic)
		{
			TotalFlow += Logic->GetFlowDirection(MyLoc);
		}
	}

	// Normalize if > 1 to prevent super speed, but keep intensity if needed usually.
	// For AddMovementInput, we usually want direction.
	return TotalFlow.GetSafeNormal();
}

// Called to bind functionality to input
void ACattleAnimal::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

