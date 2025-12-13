// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTService_DetectPlayerActions.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/OverlapResult.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "CattleGame/Weapons/Dynamite/DynamiteProjectile.h"
#include "CattleGame/Weapons/Trumpet/Trumpet.h"
#include "CattleGame/Weapons/Revolver/Revolver.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UBTService_DetectPlayerActions::UBTService_DetectPlayerActions()
{
    NodeName = "Detect Player Actions";
    Interval = 0.2f;
    RandomDeviation = 0.05f;

    // Set up blackboard key filters
    NearbyExplosiveKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayerActions, NearbyExplosiveKey), AActor::StaticClass());
    IsBeingLuredKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayerActions, IsBeingLuredKey));
    LurerActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayerActions, LurerActorKey), AActor::StaticClass());
    IsBeingScaredKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayerActions, IsBeingScaredKey));
    ScarerActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayerActions, ScarerActorKey), AActor::StaticClass());
    IsPlayerShootingKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayerActions, IsPlayerShootingKey));
    ShooterActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_DetectPlayerActions, ShooterActorKey), AActor::StaticClass());
}

void UBTService_DetectPlayerActions::TickNode(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory, float DeltaSeconds)
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

    // Detect all player actions
    DetectNearbyExplosives(OwnerComp, Animal, BlackboardComp);
    DetectTrumpetEffects(OwnerComp, Animal, BlackboardComp);
    DetectPlayerShooting(OwnerComp, Animal, BlackboardComp);
}

void UBTService_DetectPlayerActions::DetectNearbyExplosives(UBehaviorTreeComponent &OwnerComp, ACattleAnimal *Animal, UBlackboardComponent *BlackboardComp)
{
    if (NearbyExplosiveKey.SelectedKeyName == NAME_None)
    {
        return;
    }

    AActor *NearestExplosive = nullptr;
    float NearestDistance = ExplosiveDetectionRadius;

    // Find all dynamite projectiles in range
    TArray<AActor *> FoundProjectiles;
    UGameplayStatics::GetAllActorsOfClass(Animal->GetWorld(), ADynamiteProjectile::StaticClass(), FoundProjectiles);

    const FVector AnimalLocation = Animal->GetActorLocation();

    for (AActor *Projectile : FoundProjectiles)
    {
        ADynamiteProjectile *Dynamite = Cast<ADynamiteProjectile>(Projectile);
        if (!Dynamite)
        {
            continue;
        }

        // Only detect fusing dynamite (not flying)
        if (Dynamite->GetProjectileState() != EDynamiteState::Fusing)
        {
            continue;
        }

        const float Distance = FVector::Dist(AnimalLocation, Dynamite->GetActorLocation());
        if (Distance < NearestDistance)
        {
            NearestDistance = Distance;
            NearestExplosive = Dynamite;
        }
    }

    BlackboardComp->SetValueAsObject(NearbyExplosiveKey.SelectedKeyName, NearestExplosive);
}

void UBTService_DetectPlayerActions::DetectTrumpetEffects(UBehaviorTreeComponent &OwnerComp, ACattleAnimal *Animal, UBlackboardComponent *BlackboardComp)
{
    bool bIsLured = false;
    bool bIsScared = false;
    AActor *LurerActor = nullptr;
    AActor *ScarerActor = nullptr;

    const FVector AnimalLocation = Animal->GetActorLocation();

    // Find all player characters and check if they're using trumpets
    TArray<AActor *> FoundCharacters;
    UGameplayStatics::GetAllActorsOfClass(Animal->GetWorld(), ACattleCharacter::StaticClass(), FoundCharacters);

    for (AActor *CharacterActor : FoundCharacters)
    {
        ACattleCharacter *Character = Cast<ACattleCharacter>(CharacterActor);
        if (!Character)
        {
            continue;
        }

        const float Distance = FVector::Dist(AnimalLocation, Character->GetActorLocation());
        if (Distance > TrumpetDetectionRadius)
        {
            continue;
        }

        // Check if character has trumpet equipped and playing
        UInventoryComponent *Inventory = Character->GetInventoryComponent();
        if (!Inventory)
        {
            continue;
        }

        ATrumpet *Trumpet = Cast<ATrumpet>(Inventory->GetEquippedWeapon());
        if (!Trumpet || !Trumpet->IsPlaying())
        {
            continue;
        }

        if (Trumpet->IsPlayingLure())
        {
            bIsLured = true;
            LurerActor = Character;
        }
        else
        {
            bIsScared = true;
            ScarerActor = Character;
        }
    }

    // Update blackboard
    if (IsBeingLuredKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsBool(IsBeingLuredKey.SelectedKeyName, bIsLured);
    }
    if (LurerActorKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsObject(LurerActorKey.SelectedKeyName, LurerActor);
    }
    if (IsBeingScaredKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsBool(IsBeingScaredKey.SelectedKeyName, bIsScared);
    }
    if (ScarerActorKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsObject(ScarerActorKey.SelectedKeyName, ScarerActor);
    }
}

void UBTService_DetectPlayerActions::DetectPlayerShooting(UBehaviorTreeComponent &OwnerComp, ACattleAnimal *Animal, UBlackboardComponent *BlackboardComp)
{
    bool bPlayerShooting = false;
    AActor *ShooterActor = nullptr;

    const FVector AnimalLocation = Animal->GetActorLocation();

    // Find all player characters and check if they're shooting
    TArray<AActor *> FoundCharacters;
    UGameplayStatics::GetAllActorsOfClass(Animal->GetWorld(), ACattleCharacter::StaticClass(), FoundCharacters);

    const float CurrentTime = Animal->GetWorld()->GetTimeSeconds();

    for (AActor *CharacterActor : FoundCharacters)
    {
        ACattleCharacter *Character = Cast<ACattleCharacter>(CharacterActor);
        if (!Character)
        {
            continue;
        }

        const float Distance = FVector::Dist(AnimalLocation, Character->GetActorLocation());
        if (Distance > GunshotDetectionRadius)
        {
            continue;
        }

        // Check if character has revolver equipped and recently fired
        UInventoryComponent *Inventory = Character->GetInventoryComponent();
        if (!Inventory)
        {
            continue;
        }

        ARevolver *Revolver = Cast<ARevolver>(Inventory->GetEquippedWeapon());
        if (!Revolver)
        {
            continue;
        }

        // Check if fired recently (within memory time)
        const float TimeSinceLastFire = CurrentTime - Revolver->LastFireTime;
        if (TimeSinceLastFire < GunshotMemoryTime && Revolver->LastFireTime > 0.0f)
        {
            bPlayerShooting = true;
            ShooterActor = Character;
            break;
        }
    }

    // Update blackboard
    if (IsPlayerShootingKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsBool(IsPlayerShootingKey.SelectedKeyName, bPlayerShooting);
    }
    if (ShooterActorKey.SelectedKeyName != NAME_None)
    {
        BlackboardComp->SetValueAsObject(ShooterActorKey.SelectedKeyName, ShooterActor);
    }
}

FString UBTService_DetectPlayerActions::GetStaticDescription() const
{
    return TEXT("Detects explosives, trumpet effects, and shooting");
}
