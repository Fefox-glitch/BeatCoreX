#include "Core/BCXPrototypeGameMode.h"
#include "Core/BCXGameInstance.h"
#include "Core/BCXLog.h"
#include "Player/BCXPlayerCharacter.h"

ABCXPrototypeGameMode::ABCXPrototypeGameMode()
{
    DefaultPawnClass = ABCXPlayerCharacter::StaticClass();
}

void ABCXPrototypeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    UBCXGameInstance* Session = Cast<UBCXGameInstance>(GetGameInstance());
    if (!Session || !Session->BeginPrototypeSession())
    {
        ErrorMessage = TEXT("BeatCoreX requires BCXGameInstance and a valid prototype session.");
        UE_LOG(LogBeatCoreX, Error, TEXT("%s"), *ErrorMessage);
    }
}
