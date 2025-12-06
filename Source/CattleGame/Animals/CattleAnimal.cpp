// Fill out your copyright notice in the Description page of Project Settings.


#include "CattleAnimal.h"
#include "CattleGame/Weapons/Lasso/LassoableComponent.h"

// Sets default values
ACattleAnimal::ACattleAnimal()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create lassoable component for lasso target support
	LassoableComponent = CreateDefaultSubobject<ULassoableComponent>(TEXT("LassoableComponent"));
	LassoableComponent->AttachSocketName = FName("pelvis"); // Default socket for cattle

}

// Called when the game starts or when spawned
void ACattleAnimal::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACattleAnimal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ACattleAnimal::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

