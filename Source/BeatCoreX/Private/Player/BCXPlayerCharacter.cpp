#include "Player/BCXPlayerCharacter.h"
#include "Core/BCXGameInstance.h"
#include "Core/BCXLog.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformTime.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABCXPlayerCharacter::ABCXPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    Movement->JumpZVelocity = 600.0f;
    Movement->AirControl = 0.35f;
    Movement->MaxWalkSpeed = 500.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = 350.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 40.0f, 65.0f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
    PrototypeBody->SetupAttachment(GetRootComponent());
    PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeBody->SetRelativeLocation(FVector(0.0f, 0.0f, -15.0f));
    PrototypeBody->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.5f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        PrototypeBody->SetStaticMesh(Cube.Object);
    }
}

void ABCXPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    ApplyLocalMappingContext();
}

void ABCXPlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    ApplyLocalMappingContext();
}

void ABCXPlayerCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    // LocalPlayer and input component are now available; repeated calls are safe.
    ApplyLocalMappingContext();
}

void ABCXPlayerCharacter::UnPossessed()
{
    RemoveOwnedMappingContext();
    RemoveOwnedInputBindings();
    StopJumping();
    PossessionSessionGeneration = 0;
    Super::UnPossessed();
}

void ABCXPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemoveOwnedMappingContext();
    RemoveOwnedInputBindings();
    PossessionSessionGeneration = 0;
    OnAttackIntent.Clear();
    Super::EndPlay(EndPlayReason);
}

bool ABCXPlayerCharacter::ValidateInputConfiguration() const
{
    const UInputAction* Actions[] = { MoveAction.Get(), LookAction.Get(), JumpAction.Get(),
        PunchAction.Get(), KickAction.Get(), HeadbuttAction.Get() };
    if (!IsValid(GameplayMappingContext))
    {
        UE_LOG(LogBeatCoreX, Error, TEXT("%s: missing gameplay mapping context."), *GetName());
        return false;
    }

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Actions); ++Index)
    {
        const UInputAction* Action = Actions[Index];
        const EInputActionValueType Expected = Index < 2
            ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
        if (!IsValid(Action) || Action->ValueType != Expected)
        {
            UE_LOG(LogBeatCoreX, Error, TEXT("%s: missing or incorrectly typed input action at index %d."), *GetName(), Index);
            return false;
        }
        for (int32 Previous = 0; Previous < Index; ++Previous)
        {
            if (Action == Actions[Previous])
            {
                UE_LOG(LogBeatCoreX, Error, TEXT("%s: each input intent requires a distinct action."), *GetName());
                return false;
            }
        }
        const bool bMapped = GameplayMappingContext->GetMappings().ContainsByPredicate(
            [Action](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Action == Action; });
        if (!bMapped)
        {
            UE_LOG(LogBeatCoreX, Error, TEXT("%s: action %s is absent from gameplay mappings."), *GetName(), *Action->GetName());
            return false;
        }
    }
    return true;
}

void ABCXPlayerCharacter::ApplyLocalMappingContext()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController())
    {
        return;
    }
    ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
    UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Subsystem || !ValidateInputConfiguration())
    {
        RemoveOwnedMappingContext();
        return;
    }
    UBCXGameInstance* Session = Cast<UBCXGameInstance>(GetGameInstance());
    if (!Session || Session->GetSessionGeneration() <= 0)
    {
        RemoveOwnedMappingContext();
        UE_LOG(LogBeatCoreX, Error, TEXT("Cannot enable gameplay input without an explicit session."));
        return;
    }

    PossessionSessionGeneration = Session->GetSessionGeneration();
    if (AppliedSubsystem.Get() != Subsystem || AppliedMappingContext != GameplayMappingContext)
    {
        RemoveOwnedMappingContext();
    }
    AppliedSubsystem = Subsystem;
    AppliedMappingContext = GameplayMappingContext;
    if (!Subsystem->HasMappingContext(GameplayMappingContext))
    {
        FModifyContextOptions Options;
        Options.bIgnoreAllPressedKeysUntilRelease = true;
        Subsystem->AddMappingContext(GameplayMappingContext, MappingPriority, Options);
        bOwnsAppliedMapping = true;
        UE_LOG(LogBeatCoreX, Log, TEXT("BCX_MAPPING Added=%s Generation=%lld"), *GameplayMappingContext->GetName(), PossessionSessionGeneration);
    }
}

void ABCXPlayerCharacter::RemoveOwnedMappingContext()
{
    if (bOwnsAppliedMapping && AppliedSubsystem.IsValid() && IsValid(AppliedMappingContext))
    {
        AppliedSubsystem->RemoveMappingContext(AppliedMappingContext);
    }
    bOwnsAppliedMapping = false;
    AppliedSubsystem.Reset();
    AppliedMappingContext = nullptr;
}

void ABCXPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    RemoveOwnedInputBindings();
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!Input || !ValidateInputConfiguration())
    {
        RemoveOwnedMappingContext();
        UE_LOG(LogBeatCoreX, Error, TEXT("%s: input bindings disabled; Enhanced Input component/config required."), *GetName());
        return;
    }

    // Track only our bindings: a repeated setup never duplicates delegates
    // and does not erase unrelated Blueprint/controller bindings.
    BoundInputComponent = Input;
    OwnedBindingHandles.Add(Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move).GetHandle());
    OwnedBindingHandles.Add(Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look).GetHandle());
    OwnedBindingHandles.Add(Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJumpInput).GetHandle());
    OwnedBindingHandles.Add(Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumpInput).GetHandle());
    OwnedBindingHandles.Add(Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::StopJumpInput).GetHandle());
    OwnedBindingHandles.Add(Input->BindAction(PunchAction, ETriggerEvent::Started, this, &ThisClass::Punch).GetHandle());
    OwnedBindingHandles.Add(Input->BindAction(KickAction, ETriggerEvent::Started, this, &ThisClass::Kick).GetHandle());
    OwnedBindingHandles.Add(Input->BindAction(HeadbuttAction, ETriggerEvent::Started, this, &ThisClass::Headbutt).GetHandle());
}

void ABCXPlayerCharacter::RemoveOwnedInputBindings()
{
    if (BoundInputComponent.IsValid())
    {
        for (const uint32 Handle : OwnedBindingHandles)
        {
            BoundInputComponent->RemoveBindingByHandle(Handle);
        }
    }
    OwnedBindingHandles.Reset();
    BoundInputComponent.Reset();
}

void ABCXPlayerCharacter::Move(const FInputActionValue& Value)
{
    const double InputTimeSeconds = FPlatformTime::Seconds();
    if (Controller)
    {
        const FVector2D Axis = Value.Get<FVector2D>();
        const FRotator Yaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
        UE_LOG(LogBeatCoreX, VeryVerbose, TEXT("BCX_MOVE InputTimeSeconds=%.9f"), InputTimeSeconds);
    }
}

void ABCXPlayerCharacter::Look(const FInputActionValue& Value)
{
    const double InputTimeSeconds = FPlatformTime::Seconds();
    if (Controller)
    {
        const FVector2D Axis = Value.Get<FVector2D>();
        AddControllerYawInput(Axis.X);
        AddControllerPitchInput(Axis.Y);
        UE_LOG(LogBeatCoreX, VeryVerbose, TEXT("BCX_LOOK InputTimeSeconds=%.9f"), InputTimeSeconds);
    }
}

void ABCXPlayerCharacter::StartJumpInput()
{
    const double InputTimeSeconds = FPlatformTime::Seconds();
    Jump();
    UE_LOG(LogBeatCoreX, Log, TEXT("BCX_JUMP Started InputTimeSeconds=%.9f"), InputTimeSeconds);
}

void ABCXPlayerCharacter::StopJumpInput()
{
    const double InputTimeSeconds = FPlatformTime::Seconds();
    StopJumping();
    UE_LOG(LogBeatCoreX, Log, TEXT("BCX_JUMP Released InputTimeSeconds=%.9f"), InputTimeSeconds);
}

void ABCXPlayerCharacter::Punch()
{
    const double InputTimeSeconds = FPlatformTime::Seconds();
    EmitAttackIntent(EBCXAttackType::Punch, InputTimeSeconds);
}

void ABCXPlayerCharacter::Kick()
{
    const double InputTimeSeconds = FPlatformTime::Seconds();
    EmitAttackIntent(EBCXAttackType::Kick, InputTimeSeconds);
}

void ABCXPlayerCharacter::Headbutt()
{
    const double InputTimeSeconds = FPlatformTime::Seconds();
    EmitAttackIntent(EBCXAttackType::Headbutt, InputTimeSeconds);
}

void ABCXPlayerCharacter::EmitAttackIntent(EBCXAttackType AttackType, double InputTimeSeconds)
{
    if (!IsLocallyControlled() || !FMath::IsFinite(InputTimeSeconds) || InputTimeSeconds <= 0.0)
    {
        return;
    }
    UBCXGameInstance* Session = Cast<UBCXGameInstance>(GetGameInstance());
    FBCXAttackIntent Intent;
    if (!Session || !Session->AllocateAttackId(PossessionSessionGeneration, Intent.AttackId))
    {
        return;
    }
    Intent.SessionGeneration = PossessionSessionGeneration;
    Intent.InputTimeSeconds = InputTimeSeconds;
    Intent.AttackType = AttackType;
    UE_LOG(LogBeatCoreX, Log, TEXT("BCX_ATTACK Generation=%lld AttackId=%lld InputTimeSeconds=%.9f Type=%s"),
        Intent.SessionGeneration, Intent.AttackId, Intent.InputTimeSeconds,
        *UEnum::GetValueAsString(AttackType));
    OnAttackIntent.Broadcast(Intent);
}
