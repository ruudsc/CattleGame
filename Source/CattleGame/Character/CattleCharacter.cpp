// Fill out your copyright notice in the Description page of Project Settings.

#include "CattleCharacter.h"
#include "InventoryComponent.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "CattleGame/AbilitySystem/CattleAbilitySystemComponent.h"
#include "CattleGame/AbilitySystem/CattleAttributeSet.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "CattleGame/CattleGame.h"
#include "Net/UnrealNetwork.h"
#include "CattleGame/Player/CattlePlayerController.h"

// Console variable for development/testing: override mesh visibility
// Type "r.ShowAllMeshes 1" in console to show both meshes, "r.ShowAllMeshes 0" to restore normal behavior
static TAutoConsoleVariable<int32> CVarShowAllMeshes(
	TEXT("r.ShowAllMeshes"),
	0,
	TEXT("0: Normal visibility mode (owner=FP, others=TP), 1: Show both meshes for development"),
	ECVF_Cheat);

/**
 * Constructor setup notes for FPS camera:
 * - GetMesh() (inherited from ACharacter): Full body skeletal mesh visible to others
 * - FirstPersonCamera: UCameraComponent attached to capsule at (0, 0, 68) - eye level
 * - FirstPersonMesh: SKeletalMeshComponent attached to camera - only visible to owner
 * - bUsePawnControlRotation on camera: true (camera follows mouse look)
 * - bUseControllerRotationPitch: true (enable pitch/up-down look)
 * - bUseControllerRotationYaw: true (enable yaw/left-right look)
 * - bOrientRotationToMovement: false (character rotation controlled by Look input, not movement)
 */
ACattleCharacter::ACattleCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Enable replication for movement & custom replicated variables
	bReplicates = true;
	SetReplicateMovement(true);

	// Character rotation controlled by controller (mouse/gamepad), not movement
	// Pitch (up/down) and yaw (left/right) both controlled by camera
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Don't orient movement to character facing - use controller direction instead
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxAcceleration = 2400.0f;

	// Note: Third-person mesh uses inherited GetMesh() from ACharacter - configure in Blueprint
	GetMesh()->CastShadow = true;
	GetMesh()->bCastDynamicShadow = true;

	// Create first-person camera at eye level
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FirstPersonCameraOffset);
	FirstPersonCamera->bUsePawnControlRotation = true; // Camera follows controller rotation (pitch + yaw)

	// Create first-person skeletal mesh (arms/body)
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);
	FirstPersonMesh->SetRelativeLocation(FirstPersonMeshOffset);
	FirstPersonMesh->SetRelativeRotation(FirstPersonMeshRotation);

	// Enable net updates for mesh visibility in multiplayer
	FirstPersonMesh->SetIsReplicated(true);

	// Create ability system component
	AbilitySystemComponent = CreateDefaultSubobject<UCattleAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// Create inventory component for weapon management
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void ACattleCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Setup mesh visibility for FP/TP
	SetupMeshVisibility();

	// Initialize ability system component
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitializeAbilitySystem(this, this);
	}

	// Initialize inventory with default weapon classes (server only)
	if (InventoryComponent && HasAuthority())
	{
		InventoryComponent->SetDefaultWeaponClasses(
			DefaultRevolverClass,
			DefaultLassoClass,
			DefaultDynamiteClass,
			DefaultTrumpetClass);
	}

	// Grant abilities from Data Asset (server only)
	InitAbilitySystem();

	// Input mapping: if using our custom PlayerController, it owns the mapping lifecycle. Otherwise keep a safe fallback here.
	if (!Cast<ACattlePlayerController>(Controller))
	{
		if (APlayerController *PlayerController = Cast<APlayerController>(Controller))
		{
			if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
					PlayerController->GetLocalPlayer() ? PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
					UE_LOG(LogGASDebug, Verbose, TEXT("BeginPlay fallback: Added DefaultMappingContext on %s"), *GetName());
				}
			}
		}
	}
}

void ACattleCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// Re-apply mesh visibility settings when client restarts/respawns
	SetupMeshVisibility();

	// Ensure Enhanced Input mapping context is present on the local client after possession/restart.
	// Skip if using ACattlePlayerController (it will handle this).
	if (!Cast<ACattlePlayerController>(Controller))
	{
		if (APlayerController *PlayerController = Cast<APlayerController>(Controller))
		{
			if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
					PlayerController->GetLocalPlayer() ? PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
					UE_LOG(LogGASDebug, Log, TEXT("PawnClientRestart (fallback): Added DefaultMappingContext on client for %s"), *GetName());
				}
				else
				{
					UE_LOG(LogGASDebug, Warning, TEXT("PawnClientRestart: DefaultMappingContext is null for %s"), *GetName());
				}
			}
			else
			{
				UE_LOG(LogGASDebug, Warning, TEXT("PawnClientRestart: EnhancedInputLocalPlayerSubsystem not available for %s"), *GetName());
			}
		}
	}
}

void ACattleCharacter::SetMeshVisibilityMode(EMeshVisibilityMode NewMode)
{
	CurrentVisibilityMode = NewMode;
	SetupMeshVisibility();
}

EMeshVisibilityMode ACattleCharacter::ResolveMeshVisibilityMode() const
{
	EMeshVisibilityMode VisibilityMode = EMeshVisibilityMode::Normal;

	if (bOverrideMeshVisibility)
	{
		VisibilityMode = CurrentVisibilityMode;
	}
	else
	{
		VisibilityMode = IsLocallyControlled()
							 ? EMeshVisibilityMode::FirstPersonOnly
							 : EMeshVisibilityMode::ThirdPersonOnly;
	}

	if (CVarShowAllMeshes.GetValueOnGameThread() != 0)
	{
		VisibilityMode = EMeshVisibilityMode::ShowBoth;
	}

	return VisibilityMode;
}

void ACattleCharacter::SetupMeshVisibility()
{
	if (!FirstPersonMesh || !GetMesh())
	{
		return;
	}

	const bool bIsLocallyControlled = IsLocallyControlled();

	// Debug: Display IsLocallyControlled status on screen
	UE_LOG(LogGASDebug, Log, TEXT("IsLocallyControlled: %s"),
		   bIsLocallyControlled ? TEXT("TRUE") : TEXT("FALSE"));

	const EMeshVisibilityMode VisibilityMode = ResolveMeshVisibilityMode();

	// Apply visibility based on the determined mode
	switch (VisibilityMode)
	{
	case EMeshVisibilityMode::ShowBoth:
		// Show both meshes - useful for design iteration
		FirstPersonMesh->SetOnlyOwnerSee(false);
		FirstPersonMesh->SetOwnerNoSee(false);
		FirstPersonMesh->SetVisibility(true);
		FirstPersonMesh->SetCastShadow(true);
		FirstPersonMesh->bCastDynamicShadow = true;

		GetMesh()->SetOnlyOwnerSee(false);
		GetMesh()->SetOwnerNoSee(false);
		GetMesh()->SetVisibility(true);
		GetMesh()->SetCastShadow(true);
		GetMesh()->bCastDynamicShadow = true;
		break;

	case EMeshVisibilityMode::FirstPersonOnly:
		// Show only first-person mesh
		FirstPersonMesh->SetVisibility(true);
		FirstPersonMesh->SetCastShadow(false);
		FirstPersonMesh->bCastDynamicShadow = false;

		GetMesh()->SetVisibility(false);
		GetMesh()->SetCastShadow(false);
		GetMesh()->bCastDynamicShadow = false;
		break;

	case EMeshVisibilityMode::ThirdPersonOnly:
		// Show only third-person mesh
		FirstPersonMesh->SetVisibility(false);
		FirstPersonMesh->SetCastShadow(false);
		FirstPersonMesh->bCastDynamicShadow = false;

		GetMesh()->SetVisibility(true);
		GetMesh()->SetCastShadow(true);
		GetMesh()->bCastDynamicShadow = true;
		break;

	case EMeshVisibilityMode::Normal:
	default:
		// Normal mode: owner sees FP, others see TP (multiplayer-safe)
		FirstPersonMesh->SetOnlyOwnerSee(true);
		FirstPersonMesh->SetCastShadow(false);
		FirstPersonMesh->bCastDynamicShadow = false;

		GetMesh()->SetOwnerNoSee(true);
		GetMesh()->SetCastShadow(true);
		GetMesh()->bCastDynamicShadow = true;
		break;
	}

	// Set the active camera for the local player
	if (bIsLocallyControlled && FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(true);
	}
}

USkeletalMeshComponent *ACattleCharacter::GetActiveCharacterMesh() const
{
	if (!FirstPersonMesh || !GetMesh())
	{
		return nullptr;
	}

	return IsLocallyControlled() ? FirstPersonMesh.Get() : GetMesh();
}

UAbilitySystemComponent *ACattleCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool ACattleCharacter::HasGameplayTag(FGameplayTag InTag) const
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	return AbilitySystemComponent->HasMatchingGameplayTag(InTag);
}

void ACattleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACattleCharacter::OnRep_ViewRotation()
{
	// Apply yaw to actor for remote clients (pitch used for aim offsets only)
	if (!IsLocallyControlled())
	{
		FRotator NewActorRot = GetActorRotation();
		NewActorRot.Yaw = ReplicatedViewRotation.Yaw;
		SetActorRotation(NewActorRot);
	}
}

void ACattleCharacter::ServerSetViewRotation_Implementation(const FRotator &NewRotation)
{
	// Basic validation: ignore extreme values
	if (NewRotation.Pitch < -89.f || NewRotation.Pitch > 89.f)
	{
		return;
	}

	ReplicatedViewRotation = NewRotation;

	// Keep server-side controller & actor yaw in sync for replication to other clients
	if (Controller)
	{
		Controller->SetControlRotation(NewRotation);
	}

	FRotator NewActorRot = GetActorRotation();
	NewActorRot.Yaw = NewRotation.Yaw;
	SetActorRotation(NewActorRot);
}

void ACattleCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate view rotation to everyone except the owner and always call OnRep on changes we send
	DOREPLIFETIME_CONDITION_NOTIFY(ACattleCharacter, ReplicatedViewRotation, COND_SkipOwner, REPNOTIFY_Always);
}

void ACattleCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// ===== GAS ABILITY INPUT BINDING =====
		// Core character abilities (Move, Look, Jump, Crouch, Weapon Switching)
		// InputIDs 1000-1999 for character abilities, 100+ for weapon abilities
		for (const FCharacterAbilityInfo &AbilityInfo : CharacterAbilities)
		{
			if (AbilityInfo.IsValid())
			{
				const UInputAction *InputAction = AbilityInfo.InputAction;
				const int32 InputID = AbilityInfo.InputID;

				// Bind Started event -> OnAbilityInputPressed
				EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Started,
												   this, &ACattleCharacter::OnAbilityInputPressed, InputID);

				// Bind Completed event -> OnAbilityInputReleased
				EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Completed,
												   this, &ACattleCharacter::OnAbilityInputReleased, InputID);
			}
		}
	}
}

void ACattleCharacter::InitAbilitySystem()
{
	// Grant character abilities (server only)
	// These are core abilities like Move, Look, Jump, Crouch, EquipWeaponSlot
	if (HasAuthority() && AbilitySystemComponent)
	{
		constexpr int32 AbilityLevel = 1;

		for (const FCharacterAbilityInfo &AbilityInfo : CharacterAbilities)
		{
			if (AbilityInfo.IsValid())
			{
				const FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(
					AbilityInfo.GameplayAbilityClass,
					AbilityLevel,
					AbilityInfo.InputID);
				AbilitySystemComponent->GiveAbility(AbilitySpec);

				UE_LOG(LogGASDebug, Log, TEXT("InitAbilitySystem: Granted character ability %s with InputID %d"),
					   *GetNameSafe(AbilityInfo.GameplayAbilityClass),
					   AbilityInfo.InputID);
			}
		}
	}
}

void ACattleCharacter::OnAbilityInputPressed(int32 InputID)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(InputID);
	}
}

void ACattleCharacter::OnAbilityInputReleased(int32 InputID)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputReleased(InputID);
	}
}

#if WITH_EDITOR
void ACattleCharacter::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Auto-generate InputIDs for CharacterAbilities when edited
	// InputIDs start at 1000 to avoid conflicts with weapon abilities (100+)
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACattleCharacter, CharacterAbilities) ||
		PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ACattleCharacter, CharacterAbilities))
	{
		int32 NextInputID = 1000;
		for (FCharacterAbilityInfo &AbilityInfo : CharacterAbilities)
		{
			if (AbilityInfo.GameplayAbilityClass)
			{
				AbilityInfo.InputID = NextInputID++;
			}
			else
			{
				AbilityInfo.InputID = INDEX_NONE;
			}
		}
	}
}
#endif
