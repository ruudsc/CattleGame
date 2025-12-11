#include "CattleAvoidVolume.h"
#include "Components/BoxComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

ACattleAvoidVolume::ACattleAvoidVolume()
{
    // Create trigger box
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootSceneComponent);
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
    TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
    TriggerBox->SetGenerateOverlapEvents(true);

    // Default debug color for avoid volumes
    DebugColor = FColor::Red;
}

void ACattleAvoidVolume::BeginPlay()
{
    Super::BeginPlay();

    // Bind overlap events
    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACattleAvoidVolume::OnVolumeBeginOverlap);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ACattleAvoidVolume::OnVolumeEndOverlap);
}

FVector ACattleAvoidVolume::CalculateFlowDirectionInternal(const FVector &Location) const
{
    // Push cattle AWAY from the center of the volume
    FVector Center = GetActorLocation();
    FVector Direction = (Location - Center).GetSafeNormal();

    return Direction * RepulsionStrength;
}

void ACattleAvoidVolume::OnVolumeBeginOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
                                              UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (ACattleAnimal *Cattle = Cast<ACattleAnimal>(OtherActor))
    {
        HandleCattleEnter(Cattle);
    }
}

void ACattleAvoidVolume::OnVolumeEndOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
                                            UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (ACattleAnimal *Cattle = Cast<ACattleAnimal>(OtherActor))
    {
        HandleCattleExit(Cattle);
    }
}

void ACattleAvoidVolume::HandleCattleEnter(ACattleAnimal *Cattle)
{
    if (!Cattle)
    {
        return;
    }

    // Register this flow source with the cattle
    Cattle->AddActiveFlowSource(this);

    // Apply Gameplay Effect if configured
    if (ApplyEffectClass)
    {
        UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cattle);
        if (ASC)
        {
            FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
            EffectContext.AddSourceObject(this);

            FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ApplyEffectClass, 1.0f, EffectContext);
            if (SpecHandle.IsValid())
            {
                FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                ActiveEffectHandles.Add(Cattle, ActiveHandle);
            }
        }
    }
}

void ACattleAvoidVolume::HandleCattleExit(ACattleAnimal *Cattle)
{
    if (!Cattle)
    {
        return;
    }

    // Unregister this flow source from the cattle
    Cattle->RemoveActiveFlowSource(this);

    // Remove Gameplay Effect if it was applied
    if (ActiveEffectHandles.Contains(Cattle))
    {
        UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Cattle);
        if (ASC)
        {
            ASC->RemoveActiveGameplayEffect(ActiveEffectHandles[Cattle], 1);
        }
        ActiveEffectHandles.Remove(Cattle);
    }
}
