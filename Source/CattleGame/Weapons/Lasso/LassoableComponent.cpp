#include "LassoableComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "CattleGame/CattleGame.h"

ULassoableComponent::ULassoableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Create wrapper spline
	WrapSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WrapSpline"));
	// Note: We can't attach here in constructor for Actor Components easily without owner,
	// so we do it in OnRegister or the owner's constructor usually.
	// But for a component-owned subobject, it needs care.
	// Simpler: Just create it, and we attach manually in OnRegister.
}

void ULassoableComponent::OnRegister()
{
	Super::OnRegister();

	if (GetOwner())
	{
		if (!WrapSpline->GetAttachParent())
		{
			WrapSpline->SetupAttachment(GetOwner()->GetRootComponent());
			WrapSpline->RegisterComponent();
		}
	}
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

	ClearPreview();
}

#include "LassoLoopActor.h"

void ULassoableComponent::PreviewWrap()
{
#if WITH_EDITOR
	ClearPreview();

	if (!WrapLoopActorClass || !GetWorld())
	{
		UE_LOG(LogLasso, Warning, TEXT("LassoableComponent::PreviewWrap - Missing WrapLoopActorClass or World"));
		return;
	}

	UE_LOG(LogLasso, Log, TEXT("LassoableComponent::PreviewWrap - Creating preview on %s"), *GetNameSafe(GetOwner()));

	// Spawn the preview actor
	FTransform SpawnTransform = WrapSpline->GetComponentTransform();
	EditorPreviewActor = GetWorld()->SpawnActor<AActor>(WrapLoopActorClass, SpawnTransform);

	// Initialize the loop actor
	if (ALassoLoopActor *LoopActor = Cast<ALassoLoopActor>(EditorPreviewActor))
	{
		// Attach to our owner for preview (so it moves if we move owner)
		// Attaching to WrapSpline is best to keep local space correct
		LoopActor->AttachToComponent(WrapSpline, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		LoopActor->InitFromSpline(WrapSpline);
	}
#endif
}

void ULassoableComponent::ClearPreview()
{
#if WITH_EDITOR
	if (EditorPreviewActor)
	{
		EditorPreviewActor->Destroy();
		EditorPreviewActor = nullptr;
	}
#endif
}

void ULassoableComponent::OnCaptured(AActor *LassoWeaponOwner)
{
	bIsLassoed = true;
	LassoOwner = LassoWeaponOwner;

	UE_LOG(LogLasso, Warning, TEXT("LassoableComponent::OnCaptured - %s CAPTURED by %s"),
		   *GetNameSafe(GetOwner()), *GetNameSafe(LassoWeaponOwner));
	UE_LOG(LogLasso, Log, TEXT("  Owner location: %s, Capturer location: %s"),
		   GetOwner() ? *GetOwner()->GetActorLocation().ToString() : TEXT("null"),
		   LassoWeaponOwner ? *LassoWeaponOwner->GetActorLocation().ToString() : TEXT("null"));

	// Trigger blueprint event
	OnLassoCaptured();
}

void ULassoableComponent::OnReleased()
{
	UE_LOG(LogLasso, Warning, TEXT("LassoableComponent::OnReleased - %s RELEASED (was held by %s)"),
		   *GetNameSafe(GetOwner()), *GetNameSafe(LassoOwner.Get()));

	bIsLassoed = false;
	LassoOwner = nullptr;

	// Trigger blueprint event
	OnLassoReleased();
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

#if WITH_EDITOR
void ULassoableComponent::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Refresh preview when properties change
	if (EditorPreviewActor)
	{
		PreviewWrap();
	}
}
#endif
