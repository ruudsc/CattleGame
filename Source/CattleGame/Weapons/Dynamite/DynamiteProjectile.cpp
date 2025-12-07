#include "DynamiteProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "CattleGame/CattleGame.h"

ADynamiteProjectile::ADynamiteProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// Create collision sphere as root
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(25.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RootComponent = CollisionSphere;

	// Create static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionSphere);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create projectile movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 1500.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.8f;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}

void ADynamiteProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Bind collision event
	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ADynamiteProjectile::OnCollision);
	}

	// Server only: Start fuse timer
	if (HasAuthority())
	{
		CurrentState = EDynamiteState::Fusing;

		GetWorld()->GetTimerManager().SetTimer(
			ExplosionTimerHandle,
			this,
			&ADynamiteProjectile::Explode,
			FuseTime,
			false);

		UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::BeginPlay - Fuse started, explosion in %.1f seconds"), FuseTime);
	}
}

void ADynamiteProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Update tick rate if needed for continuous effects
}

void ADynamiteProjectile::Launch(const FVector &Direction, float Force)
{
	if (!ProjectileMovement)
	{
		return;
	}

	// Normalize direction and apply force
	FVector NormalizedDirection = Direction.GetSafeNormal();
	ProjectileMovement->Velocity = NormalizedDirection * Force;

	UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::Launch - Launched with velocity %s"), *ProjectileMovement->Velocity.ToString());
}

void ADynamiteProjectile::SetExplosionProperties(float Radius, float Damage)
{
	ExplosionRadius = Radius;
	ExplosionDamage = Damage;
}

void ADynamiteProjectile::SetFuseTime(float NewFuseTime)
{
	FuseTime = NewFuseTime;
}

void ADynamiteProjectile::OnCollision(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
									  FVector NormalImpulse, const FHitResult &Hit)
{
	// Only listen on server
	if (!HasAuthority())
	{
		return;
	}

	// Don't process collision with owner
	if (OtherActor == GetOwner())
	{
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::OnCollision - Hit %s"), *OtherActor->GetName());

	// For now, just log collision. Future: Could trigger explosion on impact or allow AI to eat it
	// Based on current design, we'll explode on timer only, not on impact
}

void ADynamiteProjectile::Explode()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState == EDynamiteState::Eaten)
	{
		// Eaten dynamite explodes differently (handled by AI state)
		UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::Explode - Exploding as eaten dynamite"));
	}

	// Trigger GameplayCue for explosion VFX/Audio via owner's ASC (for proper replication)
	AActor *OwnerActor = GetOwner();
	UAbilitySystemComponent *ASC = OwnerActor ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor) : nullptr;

	if (ASC)
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation();
		CueParams.Normal = FVector::UpVector;

		ASC->ExecuteGameplayCue(CattleGameplayTags::GameplayCue_Dynamite_Explode, CueParams);
		UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::Explode - GameplayCue executed via ASC at %s"), *GetActorLocation().ToString());
	}
	else
	{
		// Fallback: Try GameplayCueManager directly (won't replicate)
		if (UGameplayCueManager *CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = GetActorLocation();
			CueParams.Normal = FVector::UpVector;
			CueParams.Instigator = OwnerActor;

			CueManager->HandleGameplayCue(this, CattleGameplayTags::GameplayCue_Dynamite_Explode, EGameplayCueEvent::Executed, CueParams);
			UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::Explode - GameplayCue triggered via CueManager (no ASC) at %s"), *GetActorLocation().ToString());
		}
		else
		{
			UE_LOG(LogGASDebug, Error, TEXT("DynamiteProjectile::Explode - No ASC or GameplayCueManager found!"));
		}
	}

	// Legacy: Spawn explosion particle effect (fallback if no GameplayCue Blueprint exists)
	if (ExplosionParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticles, GetActorLocation());
	}

	// Apply explosion damage
	TArray<FHitResult> HitResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);
	FVector ExplosionCenter = GetActorLocation();

	bool bHitSomething = GetWorld()->SweepMultiByChannel(
		HitResults,
		ExplosionCenter,
		ExplosionCenter,
		FQuat::Identity,
		ECC_Pawn,
		SphereShape);

	if (bHitSomething)
	{
		for (const FHitResult &HitResult : HitResults)
		{
			if (AActor *HitActor = HitResult.GetActor())
			{
				if (HitActor == GetOwner())
					continue; // Don't damage owner

				// Apply damage
				UGameplayStatics::ApplyDamage(
					HitActor,
					ExplosionDamage,
					nullptr,
					this,
					UDamageType::StaticClass());

				UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::Explode - Damaged %s for %.0f"), *HitActor->GetName(), ExplosionDamage);
			}
		}
	}

	UE_LOG(LogGASDebug, Warning, TEXT("DynamiteProjectile::Explode - Explosion at %s, radius %.0f, damage %.0f"),
		   *GetActorLocation().ToString(), ExplosionRadius, ExplosionDamage);

	// Broadcast explosion event
	OnExploded.Broadcast();

	// Destroy the projectile
	Destroy();
}

void ADynamiteProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADynamiteProjectile, CurrentState);
}
