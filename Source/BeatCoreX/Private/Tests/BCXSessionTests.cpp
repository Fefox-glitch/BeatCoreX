#if WITH_DEV_AUTOMATION_TESTS

#include "Core/BCXGameInstance.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBCXSessionIdentityTest,
    "BeatCoreX.Core.SessionIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBCXSessionIdentityTest::RunTest(const FString& Parameters)
{
    UBCXGameInstance* Session = NewObject<UBCXGameInstance>();
    int64 Id = -1;
    TestFalse(TEXT("No identity before session start"), Session->AllocateAttackId(0, Id));
    TestEqual(TEXT("Rejected ID cleared"), Id, int64(0));
    TestTrue(TEXT("First session starts"), Session->BeginPrototypeSession());
    const int64 FirstGeneration = Session->GetSessionGeneration();
    TestTrue(TEXT("Valid input receives ID"), Session->AllocateAttackId(FirstGeneration, Id));
    TestEqual(TEXT("First ID"), Id, int64(1));
    TestFalse(TEXT("Wrong generation rejected"), Session->AllocateAttackId(FirstGeneration + 1, Id));
    TestEqual(TEXT("Rejected input does not expose an ID"), Id, int64(0));
    TestTrue(TEXT("Allocator survives change of caller/pawn"), Session->AllocateAttackId(FirstGeneration, Id));
    TestEqual(TEXT("Rejected generation did not consume ID"), Id, int64(2));
    TestTrue(TEXT("New session starts"), Session->BeginPrototypeSession());
    TestEqual(TEXT("Generation increments"), Session->GetSessionGeneration(), FirstGeneration + 1);
    TestFalse(TEXT("Old generation cannot allocate"), Session->AllocateAttackId(FirstGeneration, Id));
    TestTrue(TEXT("New generation receives independent ID"), Session->AllocateAttackId(Session->GetSessionGeneration(), Id));
    TestEqual(TEXT("IDs restart only with generation"), Id, int64(1));
    return true;
}

#endif
