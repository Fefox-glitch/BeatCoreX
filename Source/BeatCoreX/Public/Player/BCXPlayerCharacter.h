#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Player/BCXAttackIntent.h"
#include "BCXPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UEnhancedInputComponent;
struct FInputActionValue;

UCLASS()
class BEATCOREX_API ABCXPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ABCXPlayerCharacter();
    virtual void PawnClientRestart() override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(BlueprintAssignable, Category = "BeatCoreX|Input")
    FBCXAttackIntentEvent OnAttackIntent;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BeatCoreX|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BeatCoreX|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BeatCoreX|Prototype")
    TObjectPtr<UStaticMeshComponent> PrototypeBody;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input")
    TObjectPtr<UInputMappingContext> GameplayMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input", meta = (ClampMin = "0"))
    int32 MappingPriority = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input")
    TObjectPtr<UInputAction> PunchAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input")
    TObjectPtr<UInputAction> KickAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BeatCoreX|Input")
    TObjectPtr<UInputAction> HeadbuttAction;

private:
    bool ValidateInputConfiguration() const;
    void ApplyLocalMappingContext();
    void RemoveOwnedMappingContext();
    void RemoveOwnedInputBindings();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartJumpInput();
    void StopJumpInput();
    void Punch();
    void Kick();
    void Headbutt();
    void EmitAttackIntent(EBCXAttackType AttackType, double InputTimeSeconds);

    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> AppliedSubsystem;
    TWeakObjectPtr<UEnhancedInputComponent> BoundInputComponent;
    TArray<uint32> OwnedBindingHandles;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> AppliedMappingContext;

    bool bOwnsAppliedMapping = false;
    int64 PossessionSessionGeneration = 0;
};
