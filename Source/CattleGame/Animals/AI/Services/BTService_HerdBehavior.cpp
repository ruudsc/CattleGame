// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTService_HerdBehavior.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Animals/CattleAnimalMovementComponent.h"
#include "Kismet/GameplayStatics.h"

UBTService_HerdBehavior::UBTService_HerdBehavior()
{
    NodeName = "Herd Behavior";
    Interval = 0.2f;
    RandomDeviation = 0.05f;

    HerdDirectionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_HerdBehavior, HerdDirectionKey));
    HerdCountKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_HerdBehavior, HerdCountKey));
}

void UBTService_HerdBehavior::TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController *AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }

    ACattleAnimal *Animal = Cast<ACattleAnimal>(AIController->GetPawn());
    if (!Animal)
    {
        return;
    }

    UBlackboardComponent *BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    // Find nearby herd members
    TArray<ACattleAnimal *> NearbyAnimals = FindNearbyHerdMembers(Animal);
    const int32 HerdCount = NearbyAnimals.Num();

    if (HerdCountKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsInt(HerdCountKey.SelectedKeyName, HerdCount);
    }

    if (HerdCount == 0)
    {
        if (HerdDirectionKey.SelectedKeyName != NAME_None)
        {
            BlackboardComp->SetValueAsVector(HerdDirectionKey.SelectedKeyName, FVector::ZeroVector);
        }
        return;
    }

    const FVector MyLocation = Animal->GetActorLocation();
    FVector HerdCenter = FVector::ZeroVector;
    FVector AverageVelocity = FVector::ZeroVector;
    FVector SeparationForce = FVector::ZeroVector;

    // Calculate herd metrics
    for (ACattleAnimal *OtherAnimal : NearbyAnimals)
    {
        const FVector OtherLocation = OtherAnimal->GetActorLocation();
        HerdCenter += OtherLocation;

        // Get velocity for alignment
        if (UCattleAnimalMovementComponent *OtherMovement = OtherAnimal->GetAnimalMovement())
        {
            AverageVelocity += OtherMovement->Velocity;
        }

        // Separation
        const float Distance = FVector::Dist(MyLocation, OtherLocation);
        if (Distance < SeparationDistance && Distance > 0.0f)
        {
            FVector AwayDir = MyLocation - OtherLocation;
            AwayDir.Z = 0.0f;
            AwayDir.Normalize();
            SeparationForce += AwayDir * (1.0f - Distance / SeparationDistance);
        }
    }

    HerdCenter /= HerdCount;
    AverageVelocity /= HerdCount;

    // Calculate final herd direction
    FVector FinalDirection = FVector::ZeroVector;

    // Cohesion - move toward herd center
    FVector CohesionDir = HerdCenter - MyLocation;
    CohesionDir.Z = 0.0f;
    if (!CohesionDir.IsNearlyZero())
    {
        CohesionDir.Normalize();
        FinalDirection += CohesionDir * CohesionWeight;
    }

    // Alignment - match herd velocity direction
    AverageVelocity.Z = 0.0f;
    if (!AverageVelocity.IsNearlyZero())
    {
        AverageVelocity.Normalize();
        FinalDirection += AverageVelocity * AlignmentWeight;
    }

    // Separation - avoid crowding
    SeparationForce.Z = 0.0f;
    if (!SeparationForce.IsNearlyZero())
    {
        SeparationForce.Normalize();
        FinalDirection += SeparationForce * SeparationWeight;
    }

    // Normalize final direction
    if (!FinalDirection.IsNearlyZero())
    {
        FinalDirection.Normalize();
    }

    if (HerdDirectionKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsVector(HerdDirectionKey.SelectedKeyName, FinalDirection);
    }
}

TArray<ACattleAnimal *> UBTService_HerdBehavior::FindNearbyHerdMembers(ACattleAnimal *Animal) const
{
    TArray<ACattleAnimal *> Result;

    if (!Animal)
    {
        return Result;
    }

    UWorld *World = Animal->GetWorld();
    if (!World)
    {
        return Result;
    }

    const FVector MyLocation = Animal->GetActorLocation();

    TArray<AActor *> AllAnimals;
    UGameplayStatics::GetAllActorsOfClass(World, ACattleAnimal::StaticClass(), AllAnimals);

    for (AActor *Actor : AllAnimals)
    {
        ACattleAnimal *OtherAnimal = Cast<ACattleAnimal>(Actor);
        if (!OtherAnimal || OtherAnimal == Animal)
        {
            continue;
        }

        const float Distance = FVector::Dist(MyLocation, OtherAnimal->GetActorLocation());
        if (Distance <= HerdRadius)
        {
            Result.Add(OtherAnimal);
        }
    }

    return Result;
}

FString UBTService_HerdBehavior::GetStaticDescription() const
{
    return FString::Printf(TEXT("Herd behavior (radius: %.0f, C:%.1f A:%.1f S:%.1f)"),
                           HerdRadius, CohesionWeight, AlignmentWeight, SeparationWeight);
}
