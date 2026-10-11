#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BCXGameInstance.generated.h"

// Owns identity across pawn respawns; a new map/session increments generation.
UCLASS()
class BEATCOREX_API UBCXGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    bool BeginPrototypeSession();
    bool AllocateAttackId(int64 ExpectedGeneration, int64& OutAttackId);

    UFUNCTION(BlueprintPure, Category = "BeatCoreX|Session")
    int64 GetSessionGeneration() const { return SessionGeneration; }

private:
    UPROPERTY(Transient)
    int64 SessionGeneration = 0;

    int64 LastAttackId = 0;
};
