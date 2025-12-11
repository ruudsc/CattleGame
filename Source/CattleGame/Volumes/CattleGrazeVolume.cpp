#include "CattleGrazeVolume.h"
#include "Components/BoxComponent.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"

ACattleGrazeVolume::ACattleGrazeVolume()
{
    // Create trigger box
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootSceneComponent);
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
    TriggerBox->SetBoxExtent(FVector(500.0f, 500.0f, 100.0f));
    TriggerBox->SetGenerateOverlapEvents(true);

    // Default debug color for graze volumes
    DebugColor = FColor::Green;
}

void ACattleGrazeVolume::BeginPlay()
{
    Super::BeginPlay();

    // Bind overlap events
    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACattleGrazeVolume::OnVolumeBeginOverlap);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ACattleGrazeVolume::OnVolumeEndOverlap);
}

FVector ACattleGrazeVolume::CalculateFlowDirectionInternal(const FVector &Location) const
{
    // Grazing zones don't provide flow - cattle move freely within
    return FVector::ZeroVector;
}

void ACattleGrazeVolume::OnVolumeBeginOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
                                              UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (ACattleAnimal *Cattle = Cast<ACattleAnimal>(OtherActor))
    {
        HandleCattleEnter(Cattle);
    }
}

void ACattleGrazeVolume::OnVolumeEndOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
                                            UPrimitiveComponent *OtherComp, int32 OtherBodyIndex)
{
    if (ACattleAnimal *Cattle = Cast<ACattleAnimal>(OtherActor))
    {
        HandleCattleExit(Cattle);
    }
}

void ACattleGrazeVolume::HandleCattleEnter(ACattleAnimal *Cattle)
{
    if (!Cattle)
    {
        return;
    }

    // Register this flow source with the cattle (even though we provide no flow)
    Cattle->AddActiveFlowSource(this);

    // Apply Gameplay Effect if configured (e.g., GE_Cattle_GrazingState)
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

void ACattleGrazeVolume::HandleCattleExit(ACattleAnimal *Cattle)
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

void ACattleGrazeVolume::DrawDebugVisualization() const
{
    if (!GetWorld() || !TriggerBox)
    {
        return;
    }

    // Draw the box extent instead of a sphere for graze volumes
    FVector BoxExtent = TriggerBox->GetScaledBoxExtent();
    FVector Center = TriggerBox->GetComponentLocation();

    DrawDebugBox(GetWorld(), Center, BoxExtent, TriggerBox->GetComponentQuat(), DebugColor, false, -1.0f, 0, 2.0f);

    // Draw a "G" marker at center to indicate grazing zone
    DrawDebugString(GetWorld(), Center + FVector(0, 0, BoxExtent.Z + 50.0f), TEXT("GRAZE"), nullptr, DebugColor, -1.0f, true, 2.0f);
}
