#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BCXPrototypeGameMode.generated.h"

UCLASS()
class BEATCOREX_API ABCXPrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ABCXPrototypeGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
