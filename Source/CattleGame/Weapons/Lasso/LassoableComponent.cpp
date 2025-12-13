#include "LassoableComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "CattleGame/CattleGame.h"

ULassoableComponent::ULassoableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULassoableComponent::BeginPlay()
{
	Super::BeginPlay();

	// Automatically add the lassoable tag to owner
	if (AActor *Owner = GetOwner())
	{
		Owner->Tags.AddUnique(FName("Target.Lassoable"));
		UE_LOG(LogLasso, Log, TEXT("LassoableComponent::BeginPlay - Added 'Target.Lassoable' tag to %s"),
			   *GetNameSafe(Owner));
	}
}

void ULassoableComponent::OnCaptured(AActor *LassoWeaponOwner)
{
	bIsLassoed = true;
	LassoOwner = LassoWeaponOwner;

	UE_LOG(LogLasso, Log, TEXT("LassoableComponent::OnCaptured - %s CAPTURED by %s"),
		   *GetNameSafe(GetOwner()), *GetNameSafe(LassoWeaponOwner));

	// Trigger blueprint event
	OnLassoCaptured();

	// Broadcast native delegate for C++ listeners
	OnCapturedDelegate.Broadcast(LassoWeaponOwner);
}

void ULassoableComponent::OnReleased()
{
	UE_LOG(LogLasso, Log, TEXT("LassoableComponent::OnReleased - %s RELEASED (was held by %s)"),
		   *GetNameSafe(GetOwner()), *GetNameSafe(LassoOwner.Get()));

	bIsLassoed = false;
	LassoOwner = nullptr;

	// Trigger blueprint event
	OnLassoReleased();

	// Broadcast native delegate for C++ listeners
	OnReleasedDelegate.Broadcast();
}

FTransform ULassoableComponent::GetLoopAttachTransform() const
{
	AActor *Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogLasso, Warning, TEXT("LassoableComponent::GetLoopAttachTransform - No owner!"));
		return FTransform::Identity;
	}

	FTransform SocketTransform = FTransform::Identity;

	// Try to get socket transform from skeletal mesh
	if (ACharacter *Character = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent *Mesh = Character->GetMesh())
		{
			if (Mesh->DoesSocketExist(AttachSocketName))
			{
				SocketTransform = Mesh->GetSocketTransform(AttachSocketName);
				UE_LOG(LogLasso, Verbose, TEXT("LassoableComponent::GetLoopAttachTransform - Using socket '%s' on %s"),
					   *AttachSocketName.ToString(), *GetNameSafe(Owner));
			}
			else
			{
				// Fallback to actor location if socket doesn't exist
				SocketTransform.SetLocation(Owner->GetActorLocation());
				SocketTransform.SetRotation(Owner->GetActorQuat());
				UE_LOG(LogLasso, Verbose, TEXT("LassoableComponent::GetLoopAttachTransform - Socket '%s' not found, using actor location"),
					   *AttachSocketName.ToString());
			}
		}
	}
	else
	{
		// Non-character actors: use actor transform
		SocketTransform.SetLocation(Owner->GetActorLocation());
		SocketTransform.SetRotation(Owner->GetActorQuat());
		UE_LOG(LogLasso, Verbose, TEXT("LassoableComponent::GetLoopAttachTransform - Non-character, using actor transform"));
	}

	// Apply offset and rotation
	FTransform OffsetTransform(AttachRotation, AttachOffset, LoopScale);
	return OffsetTransform * SocketTransform;
}
