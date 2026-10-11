#include "Core/BCXGameInstance.h"
#include "Core/BCXLog.h"

bool UBCXGameInstance::BeginPrototypeSession()
{
    check(IsInGameThread());
    if (SessionGeneration == MAX_int64)
    {
        UE_LOG(LogBeatCoreX, Error, TEXT("Session generation exhausted; session rejected."));
        return false;
    }

    ++SessionGeneration;
    LastAttackId = 0;
    UE_LOG(LogBeatCoreX, Log, TEXT("BCX_SESSION Generation=%lld"), SessionGeneration);
    return true;
}

bool UBCXGameInstance::AllocateAttackId(int64 ExpectedGeneration, int64& OutAttackId)
{
    check(IsInGameThread());
    OutAttackId = 0;
    if (ExpectedGeneration <= 0 || ExpectedGeneration != SessionGeneration || LastAttackId == MAX_int64)
    {
        UE_LOG(LogBeatCoreX, Warning, TEXT("Attack identity rejected: invalid session or exhausted IDs."));
        return false;
    }

    OutAttackId = ++LastAttackId;
    return true;
}
