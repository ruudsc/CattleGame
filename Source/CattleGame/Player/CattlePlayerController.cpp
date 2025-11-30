#include "CattlePlayerController.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "CattleGame/CattleGame.h"

ACattlePlayerController::ACattlePlayerController()
{
    bReplicates = true; // Controller exists on server and owning client
}

void ACattlePlayerController::ApplyDefaultInputContext(const TCHAR *ReasonTag)
{
    if (!IsLocalController())
    {
        return; // Only local players have input
    }

    // Determine which context to apply: prefer controller's, else fall back to pawn's setting
    UInputMappingContext *ContextToApply = DefaultMappingContext;
    if (!ContextToApply)
    {
        if (APawn *P = GetPawn())
        {
            if (ACattleCharacter *CC = Cast<ACattleCharacter>(P))
            {
                ContextToApply = CC->GetDefaultMappingContext();
                if (ContextToApply)
                {
                    UE_LOG(LogGASDebug, Verbose, TEXT("%s: Falling back to Character's DefaultMappingContext on %s"), ReasonTag ? ReasonTag : TEXT("ApplyDefaultInputContext"), *GetName());
                }
            }
        }
    }

    if (!ContextToApply)
    {
        UE_LOG(LogGASDebug, Warning, TEXT("%s: No mapping context available (controller and pawn are null) on %s"), ReasonTag ? ReasonTag : TEXT("ApplyDefaultInputContext"), *GetName());
        return;
    }

    // Avoid duplicate adds; only re-add if the context changed
    if (LastAppliedContext == ContextToApply)
    {
        UE_LOG(LogGASDebug, Verbose, TEXT("%s: Mapping context already applied on %s"), ReasonTag ? ReasonTag : TEXT("ApplyDefaultInputContext"), *GetName());
        return;
    }

    if (ULocalPlayer *LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            // Remove previous mapping if any (safe to call with null)
            if (LastAppliedContext)
            {
                Subsystem->RemoveMappingContext(LastAppliedContext);
            }

            Subsystem->AddMappingContext(ContextToApply, MappingPriority);
            LastAppliedContext = ContextToApply;
            UE_LOG(LogGASDebug, Log, TEXT("%s: Added MappingContext '%s' with priority %d on %s"), ReasonTag ? ReasonTag : TEXT("ApplyDefaultInputContext"), *GetNameSafe(ContextToApply), MappingPriority, *GetName());
        }
        else
        {
            UE_LOG(LogGASDebug, Warning, TEXT("%s: EnhancedInputLocalPlayerSubsystem not available for %s"), ReasonTag ? ReasonTag : TEXT("ApplyDefaultInputContext"), *GetName());
        }
    }
}

void ACattlePlayerController::BeginPlay()
{
    Super::BeginPlay();
    ApplyDefaultInputContext(TEXT("BeginPlay"));
}

void ACattlePlayerController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);
    ApplyDefaultInputContext(TEXT("OnPossess"));
}

void ACattlePlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    ApplyDefaultInputContext(TEXT("OnRep_PlayerState"));
}
