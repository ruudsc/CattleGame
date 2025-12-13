// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAnimal.h"
#include "CattleAnimalMovementComponent.h"
#include "AI/CattleAIController.h"
#include "CattleGame/Weapons/Lasso/LassoableComponent.h"
#include "CattleGame/AbilitySystem/CattleAbilitySystemComponent.h"
#include "CattleGame/AbilitySystem/AnimalAttributeSet.h"
#include "Areas/CattleAreaSubsystem.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"

ACattleAnimal::ACattleAnimal(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCattleAnimalMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Enable tick
	PrimaryActorTick.bCanEverTick = true;

	// Get reference to custom movement component
	AnimalMovement = Cast<UCattleAnimalMovementComponent>(GetCharacterMovement());

	// Create ability system component
	AbilitySystemComponent = CreateDefaultSubobject<UCattleAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Create animal attributes
	AnimalAttributes = CreateDefaultSubobject<UAnimalAttributeSet>(TEXT("AnimalAttributes"));

	// Create lassoable component for lasso target support
	LassoableComponent = CreateDefaultSubobject<ULassoableComponent>(TEXT("LassoableComponent"));
	LassoableComponent->AttachSocketName = LassoAttachSocket;

	// Default AI settings
	AIControllerClass = ACattleAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent *ACattleAnimal::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ACattleAnimal::BeginPlay()
{
	Super::BeginPlay();

	// Cache the area subsystem
	if (UWorld *World = GetWorld())
	{
		CachedAreaSubsystem = World->GetSubsystem<UCattleAreaSubsystem>();
	}

	// Bind lasso capture/release delegates
	if (LassoableComponent)
	{
		LassoableComponent->OnCapturedDelegate.AddDynamic(this, &ACattleAnimal::OnLassoCaptured);
		LassoableComponent->OnReleasedDelegate.AddDynamic(this, &ACattleAnimal::OnLassoReleased);
	}

	// Initialize ability system
	InitializeAbilitySystem();

	// Set initial movement mode
	if (AnimalMovement)
	{
		AnimalMovement->SetMovementMode_Walking();
	}
}

void ACattleAnimal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update area influences periodically
	AreaUpdateTimer += DeltaTime;
	if (AreaUpdateTimer >= AreaUpdateInterval)
	{
		AreaUpdateTimer = 0.0f;
		UpdateAreaInfluences();
	}

	// Process influences and decay fear
	ProcessAreaInfluences(DeltaTime);
	DecayFear(DeltaTime);
}

void ACattleAnimal::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Animals don't typically have player input, but AI controllers can use this
}

void ACattleAnimal::PossessedBy(AController *NewController)
{
	Super::PossessedBy(NewController);

	// Initialize ability actor info for server
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		GrantDefaultAbilities();
		ApplyDefaultEffects();
	}
}

void ACattleAnimal::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Initialize ability actor info
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Register attribute set
	if (AnimalAttributes)
	{
		AbilitySystemComponent->AddAttributeSetSubobject(AnimalAttributes.Get());
	}
}

void ACattleAnimal::GrantDefaultAbilities()
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility> &AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
			AbilitySystemComponent->GiveAbility(Spec);
		}
	}
}

void ACattleAnimal::ApplyDefaultEffects()
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect> &EffectClass : DefaultEffects)
	{
		if (EffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1, EffectContext);
			if (SpecHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void ACattleAnimal::UpdateAreaInfluences()
{
	if (!CachedAreaSubsystem)
	{
		return;
	}

	const FVector Location = GetActorLocation();

	// Get primary area influence (highest priority)
	CurrentInfluence = CachedAreaSubsystem->GetPrimaryAreaAtLocation(Location);

	// Also get flow direction
	if (AnimalMovement)
	{
		const FVector FlowDir = CachedAreaSubsystem->GetFlowDirectionAtLocation(Location);
		AnimalMovement->SetFlowDirection(FlowDir);
	}
}

FCattleAreaInfluence ACattleAnimal::GetCurrentAreaInfluence() const
{
	return CurrentInfluence;
}

void ACattleAnimal::ProcessAreaInfluences(float DeltaTime)
{
	if (!AnimalMovement || !CurrentInfluence.IsValid())
	{
		return;
	}

	// Apply area influence to movement
	AnimalMovement->SetAreaInfluence(CurrentInfluence.InfluenceDirection, CurrentInfluence.SpeedModifier);

	// Update movement mode based on state
	if (IsPanicked())
	{
		AnimalMovement->SetMovementMode_Panic();
	}
	else if (CurrentInfluence.AreaType == ECattleAreaType::Graze)
	{
		AnimalMovement->SetMovementMode_Grazing();
	}
	else
	{
		AnimalMovement->SetMovementMode_Walking();
	}
}

void ACattleAnimal::DecayFear(float DeltaTime)
{
	if (!AnimalAttributes)
	{
		return;
	}

	const float CurrentFear = AnimalAttributes->GetFear();
	if (CurrentFear > 0.0f)
	{
		float DecayRate = AnimalAttributes->GetFearDecayRate();

		// Faster decay in graze areas
		if (CurrentInfluence.IsValid() && CurrentInfluence.AreaType == ECattleAreaType::Graze)
		{
			// Get the graze area's fear decay multiplier (default 2x)
			DecayRate *= 2.0f;
		}

		const float NewFear = FMath::Max(0.0f, CurrentFear - DecayRate * DeltaTime);
		AnimalAttributes->SetFear(NewFear);
	}
}

void ACattleAnimal::ApplyPhysicsImpulse(FVector Impulse, bool bVelocityChange)
{
	if (AnimalMovement)
	{
		AnimalMovement->AddPhysicsImpulse(Impulse, bVelocityChange);
	}
}

void ACattleAnimal::AddFear(float Amount)
{
	if (AnimalAttributes && Amount > 0.0f)
	{
		const float CurrentFear = AnimalAttributes->GetFear();
		const float MaxFear = AnimalAttributes->GetMaxFear();
		const float NewFear = FMath::Clamp(CurrentFear + Amount, 0.0f, MaxFear);
		AnimalAttributes->SetFear(NewFear);
	}
}

void ACattleAnimal::AddCalm(float Amount)
{
	if (AnimalAttributes && Amount > 0.0f)
	{
		// Calm reduces fear based on lure susceptibility
		const float Susceptibility = AnimalAttributes->GetLureSusceptibility();
		const float CurrentFear = AnimalAttributes->GetFear();
		const float NewFear = FMath::Max(0.0f, CurrentFear - Amount * Susceptibility);
		AnimalAttributes->SetFear(NewFear);

		// Also increase calm level
		const float CurrentCalm = AnimalAttributes->GetCalmLevel();
		AnimalAttributes->SetCalmLevel(CurrentCalm + Amount * Susceptibility);
	}
}

bool ACattleAnimal::IsPanicked() const
{
	if (AnimalAttributes)
	{
		return AnimalAttributes->IsPanicked();
	}
	return false;
}

float ACattleAnimal::GetFearPercent() const
{
	if (AnimalAttributes)
	{
		return AnimalAttributes->GetFearPercent();
	}
	return 0.0f;
}

void ACattleAnimal::OnLassoCaptured(AActor *LassoOwner)
{
	bIsLassoed = true;

	// Add fear when captured
	AddFear(LassoFearAmount);

	// Switch to restricted movement mode while lassoed
	if (AnimalMovement)
	{
		// Use panic speed but movement is constrained by lasso
		AnimalMovement->SetMovementMode_Panic();
	}
}

void ACattleAnimal::OnLassoReleased()
{
	bIsLassoed = false;

	// Return to normal movement mode
	if (AnimalMovement)
	{
		if (IsPanicked())
		{
			AnimalMovement->SetMovementMode_Panic();
		}
		else
		{
			AnimalMovement->SetMovementMode_Walking();
		}
	}
}