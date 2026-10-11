#pragma once

#include "CoreMinimal.h"
#include "BCXAttackIntent.generated.h"

UENUM(BlueprintType)
enum class EBCXAttackType : uint8
{
    Punch,
    Kick,
    Headbutt
};

// Intention only: this prototype does not judge timing or apply damage.
// Signed positive IDs allow lossless Blueprint exposure (no int32 truncation).
USTRUCT(BlueprintType)
struct BEATCOREX_API FBCXAttackIntent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BeatCoreX|Input")
    int64 SessionGeneration = 0;

    UPROPERTY(BlueprintReadOnly, Category = "BeatCoreX|Input")
    int64 AttackId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "BeatCoreX|Input")
    double InputTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "BeatCoreX|Input")
    EBCXAttackType AttackType = EBCXAttackType::Punch;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBCXAttackIntentEvent, const FBCXAttackIntent&, Intent);
