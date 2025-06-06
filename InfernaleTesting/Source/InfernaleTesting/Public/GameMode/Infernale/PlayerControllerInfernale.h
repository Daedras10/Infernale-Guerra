// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Component/PlayerController/BuildComponent.h"
#include "DataAsset/BuildingListDataAsset.h"
#include "GameFramework/PlayerController.h"
#include "Player/InfernalePawn.h"
#include "Structs/ReplicationStructs.h"
#include "Structs/SimpleStructs.h"
#include "PlayerControllerInfernale.generated.h"


class AMainBuilding;
struct FFastArraySerializer;
class ABoss;
class AUnitActorManager;
struct FBattleInfo;
class AAmalgamVisualisationManager;
class UTransmutationComponent;
class UGameSettingsDataAsset;
struct FOwner;
class UFluxComponent;
class UUIComponent;
class UInteractionComponent;
class UPlayerNetworkComponent;
class UBuildComponent;
class ABreach;
struct FBuildingStruct;
class AInfernalePawn;
struct FInputActionValue;
class UInfernaleInputDataAsset;
class UGRGameInputDataAsset;
class UInputMappingContext;
// Input Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMousePrimaryStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMousePrimaryEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMousePrimaryTriggered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseSecondaryStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseSecondaryEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseSecondaryTriggered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseFluxModeStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMouseMoved, FVector2D, MousePosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoveTriggered, FVector2D, Move);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoveCompleted, FVector2D, Move);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCameraProfileTriggered, FVector2D, Move);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCameraProfileCompleted, FVector2D, Move);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnableMoveActionTriggered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnableMoveActionCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnableMoveActionCanceled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLookTriggered, FVector2D, Look);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCameraHeightTriggered, float, CameraHeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnableRotationStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnableRotationCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseScrollUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseScrollDown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseScrollStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMouseScrollEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEscapeStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBuildBuilding, ABreach*, Breach, FBuildingStruct, Building);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPKeyStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTransmutationToogle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSpacePressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSpaceReleased);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerControllerBool, bool, bValue);


// Event Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerReadyDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInteractionDone);

// Draft
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyTurnChanged, bool, bMyTurn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDraftEnded);


USTRUCT()
struct FPlayerControllerInfo
{
	GENERATED_USTRUCT_BODY();
public:
	FPlayerControllerInfo();

public:
	UPROPERTY() APlayerControllerInfernale* PlayerController;
	UPROPERTY() FVector PlayerLocation;
	UPROPERTY() TArray<FDataForVisualisation> DataForVisualisations;
	//UPROPERTY() FVisualDataArray DataForVisualisations;
	UPROPERTY() TArray<FMassEntityHandle> EntityHandlesToHide = TArray<FMassEntityHandle>();
};

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API APlayerControllerInfernale : public APlayerController
{
	GENERATED_BODY()

public:
	APlayerControllerInfernale();
	
	void SetPlayerInputComponent(UInputComponent* NewPlayerInputComponent);
	void SetInfernalePawn(AInfernalePawn* NewInfernalePawn);
	AInfernalePawn* GetInfernalePawn() const;
	void SetBuildingPermanent(ABuilding* Building, bool IsPermanent);
	void RemoveLoadingScreen();
	UUIComponent* GetUIComponent() const;
	UTransmutationComponent* GetTransmutationComponent() const;
	UBuildComponent* GetBuildComponent() const;
	AUnitActorManager* GetUnitActorManager() const;
	void MoveToSpawn(FVector MoveToSpawn);

	UFUNCTION(BlueprintCallable) void CallOnPrimaryStart();
	UFUNCTION(BlueprintCallable) void CallOnPrimaryEnd();

	void BossAddToSpawnCharge(ABoss* Boss, float Charge);

	UFUNCTION(BlueprintCallable) bool UseFluxMode();
	UFUNCTION(BlueprintCallable) UFluxComponent* GetFluxComponent() const;
	UFUNCTION(BlueprintCallable) UInteractionComponent* GetInteractionComponent() const;
	UFUNCTION(BlueprintCallable) void CallInteractionDone();
	UFUNCTION(BlueprintCallable) void CallFluxFluxModeStart();
	UFUNCTION(BlueprintCallable) void CallTransmutationToogle();
	UFUNCTION(BlueprintCallable) void AskBuildBuilding(ABreach* Breach, FBuildingStruct Building);
	UFUNCTION(BlueprintCallable) void EnableInputs(bool newVal);

	// Draft
	UFUNCTION(BlueprintCallable, Server, Reliable) void ServerActionChooseBase();
	UFUNCTION(BlueprintCallable) bool IsInDraftMode();
	UFUNCTION(BlueprintCallable) bool IsMyTurn();
	UFUNCTION(Client, Reliable) void StartTurnOwning();
	UFUNCTION(Client, Reliable) void EndTurnOwning();
	UFUNCTION(Client,Reliable) void EndDraftOwning();
	UFUNCTION(Client, Reliable) void EndCycleOwning();
	UFUNCTION(Server, Reliable) void ChangeBaseOwnerServer(AActor* ActorInteractable, ETeam NewOwnerTeam, EPlayerOwning NewOwnerPlayer);

	bool IsCtrlPressed() const;
	bool IsShiftPressed() const;

	ETeam GetTeam() const;
	EPlayerOwning GetPlayerOwning() const;
	UFUNCTION(BlueprintCallable, BlueprintPure) FOwner GetOwnerInfo() const;
	UFUNCTION(BlueprintCallable, BlueprintPure) bool IsAllowedToCheatBP() const;

	void AskReplicateCenterPoint(FVector NewCenterPoint);
	FVector GetCameraCenterPoint() const;
	void UpdateUnits(const TArray<FDataForVisualisation>& UnitsToReplicate, const TArray<FMassEntityHandle>& EntitiesToHide);
	void DoVfx(FBattleInfo BattleInfo);
	void DoDeathVfx(FDeathInfo DeathInfo);

	bool IsInDraftMode() const;
	void SetDraftMode(bool newVal);

	FPlayerInfo GetPlayerInfo() const;
	void SetPlayerInfo(const FPlayerInfo NewPlayerInfo);
	void MainBuildingInteracted(AMainBuilding* MainBuilding);

	bool ShouldReplicateUnitToHide() const;
	int GetFramesSinceLastReplicate() const;
	bool ShouldReplicateUnitToHideThisFrame() const;

	/* Cheats */
	UFUNCTION(Exec) void IG_AddSouls(float Amount);
	UFUNCTION(Exec) void IG_AddSoulsPlayer(int PlayerID, float Amount);
	
	UFUNCTION(Exec) void IG_AddVictoryPoints(float Amount);
	UFUNCTION(Exec) void IG_AddVictoryPointsTeam(int TeamID, float Amount);
	
	UFUNCTION(Exec) void IG_ChangeOwnership(FString BaseName);
	UFUNCTION(Exec) void IG_ChangeOwnershipPlayer(int PlayerID, int TeamID, const FString& BaseName);

	UFUNCTION(Exec) void IG_EndGame();
	UFUNCTION(Exec) void IG_StartNewGame(float NewTimeLeft);
	UFUNCTION(Exec) void IG_StartNewGameFull();
	UFUNCTION(Exec) void IG_CheatMidGame();

	UFUNCTION(Exec) void Secret();
	UFUNCTION(Exec) void UnSecret();

	UFUNCTION(BlueprintImplementableEvent) void LaunchNewGameBP();

protected:
	/* UE Functions override */
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaTime) override;
	UFUNCTION(Client, Reliable) void DrawSphereAtPointClient(FVector Location);
	
	void SetupInputMappings();
	void BindInputs();
	
	// Input Actions
	void MoveActionTriggered(const FInputActionValue& Value);
	void MoveActionCompleted(const FInputActionValue& Value);
	void EnableMoveActionTriggeredFunc();
	void EnableMoveActionCompletedFunc();
	void LookActionTriggered(const FInputActionValue& Value);
	void CameraHeightActionTriggered(const FInputActionValue& Value);
	void EnableRotationActionStarted(const FInputActionValue& Value);
	void EnableRotationActionCompleted(const FInputActionValue& Value);
	void PrimaryActionStarted();
	void PrimaryActionEnded();
	void PrimaryActionTriggered();
	void SecondaryActionStarted();
	void SecondaryActionEnded();
	void SecondaryActionTriggered();
	void FluxModeActionStarted();
	void MouseScroll(const FInputActionValue& Value);
	void StartScroll();
	void StopScroll();
	void ResetScrollTimer();
	void EscapeActionStarted();
	void ControlActionStarted();
	void ControlActionCompleted();
	void ShiftActionStarted();
	void ShiftActionCompleted();
	void PActionStarted();
	void PActionCompleted();
	void CameraProfilesActionTriggered(const FInputActionValue& Value);
	void CameraProfilesActionCompleted(const FInputActionValue& Value);
	void DotDotActionStarted();
	void IncreaseBatch();
	void DecreaseBatch();
	void EnableLANActionStarted();
	void KeyboardRotationActionTriggered(const FInputActionValue& InputActionValue);
	void OnResetRotationStarted();

	void OnSpacePressed();
	void OnSpaceReleased();
	UFUNCTION(BlueprintNativeEvent) void OnSpacePressedBP();
	UFUNCTION(BlueprintNativeEvent) void OnSpaceReleasedBP();
	UFUNCTION(BlueprintNativeEvent) void OnEscapeStartedBP();


	void UnsubscribeClickEvents();
	void SubscribeClickEvents();

	UFUNCTION() void OnFluxModeChanged(const bool bIsFluxMode);
	UFUNCTION() void OnPawnMoved(const FVector Location);
	UFUNCTION() void OnTransmutationMode(bool bIsTransmutationMode);

	//Draft
	UFUNCTION() void OnMyTurnChanged(bool bMyTurn);
	UFUNCTION() void OnDraftEnded();
	UFUNCTION() void SetDraftEnded();
	UFUNCTION() void SetMyTurn(bool bMyTurn);
	
	UFUNCTION(Server, Reliable) void BuildBuildingServer(ABreach* Breach, FBuildingStruct Building);
	UFUNCTION(Server, Reliable) void SetBuildingPermanentServer(ABuilding* Building, bool IsPermanent);
	UFUNCTION(Server, Unreliable) void ReplicateCenterPointServerUnreliable(FVector NewCenterPoint);
	UFUNCTION(Server, Reliable) void IncreaseBatchServer();
	UFUNCTION(Server, Reliable) void DecreaseBatchServer();

	/* Cheats */
	UFUNCTION(Server, Reliable) void AddSoulsServer(float Amount);
	UFUNCTION(Server, Reliable) void AddSoulsPlayerServer(int PlayerID, float Amount);
	UFUNCTION(Server, Reliable) void AddVictoryPointsServer(float Amount);
	UFUNCTION(Server, Reliable) void AddVictoryPointsTeamServer(int TeamID, float Amount);
	UFUNCTION(Server, Reliable) void ChangeOwnershipServer(const FString& BaseName);
	UFUNCTION(Server, Reliable) void ChangeOwnershipPlayerServer(int PlayerID, int TeamID, const FString& BaseName);
	UFUNCTION(Server, Reliable) void EndGameServer();
	UFUNCTION(Server, Reliable) void StartNewGameServer(float NewTimeLeft);
	UFUNCTION(Server, Reliable) void CheatMidGameServer();
	
	
	UFUNCTION(Client, Reliable) void RemoveLoadingScreenOwning();
	UFUNCTION(Client, Reliable) void EnableInputsOwning(const bool bIsInputsAllowed);

	/* TArray<FDataForVisualisation> */
	
	void UpdateUnitsSelf(const TArray<FDataForVisualisation>& UnitsToReplicate, const TArray<FMassEntityHandle>& EntitiesToHide);
	UFUNCTION(Client, Unreliable) void UpdateUnitsOwner(const TArray<FDataForVisualisation>& UnitsToReplicate, const TArray<FMassEntityHandle>& EntitiesToHide);
	UFUNCTION(Client, Unreliable) void UpdateUnitsInfoOwner(const TArray<FDataForVisualisation>& UnitsToReplicate);
	UFUNCTION(Client, Reliable) void UpdateUnitsToHideOwner(const TArray<FMassEntityHandle>& EntitiesToHide);
	UFUNCTION(Client, Unreliable) void DoVFXOwner(const FBattleInfo BattleInfo);
	UFUNCTION(Client, Unreliable) void DoDeathVFXOwner(const FDeathInfo DeathInfo);

	UFUNCTION(BlueprintCallable) void SyncGameSettings();
	UFUNCTION() void RemoveLodingScreenAfterDelay();

	// OSKUR LE BOSS
	UFUNCTION(Server, Reliable) void BossAddToSpawnChargeServer(ABoss* Boss, float Charge);
	
public:
	// Input Delegates
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMousePrimaryStart MousePrimaryStart;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMousePrimaryEnd MousePrimaryEnd;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMousePrimaryEnd MousePrimaryTriggered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseSecondaryStart MouseSecondaryStart;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseSecondaryEnd MouseSecondaryEnd;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseSecondaryEnd MouseSecondaryTriggered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseFluxModeStart FluxModeStart;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMoveTriggered MoveTriggered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMoveCompleted MoveCompleted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FCameraProfileTriggered CameraProfileTriggered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FCameraProfileCompleted CameraProfileCompleted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FEnableMoveActionTriggered EnableMoveActionTriggered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FEnableMoveActionCompleted EnableMoveActionCompleted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FEnableMoveActionCanceled EnableMoveActionCanceled;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FLookTriggered LookTriggered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FCameraHeightTriggered ScrollTriggered;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FEnableRotationStarted EnableRotationStarted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FEnableRotationCompleted EnableRotationCompleted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseScrollUp MouseScrollUp;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseScrollDown MouseScrollDown;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseScrollStarted MouseScrollStarted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMouseScrollEnded MouseScrollEnded;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FEscapeStarted EscapeStarted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FPKeyStarted PKeyStarted;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FPKeyStarted ShiftModeChanged;

	// Event Delegates
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FPlayerReadyDelegate PlayerReady;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FPawnMoved PawnMoved;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FInteractionDone InteractionDone;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FBuildBuilding BuildBuilding;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FTransmutationToogle TransmutationToogle;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FPlayerControllerBool TransmutationModeChanged;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMyTurnChanged MyTurnChanged;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FDraftEnded DraftEnded;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FSpacePressed SpacePressed;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FSpaceReleased SpaceReleased;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FMainBuildingDelegate MainBuildingInteractedDelegate;
	
	// Escape Menu
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsEscapeMenuOpen = false;
	
protected:
	// Components
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UInteractionComponent* InteractionComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UBuildComponent* BuildComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UPlayerNetworkComponent* PlayerNetworkComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UUIComponent* UIComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UFluxComponent* FluxComponent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UTransmutationComponent* TransmutationComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated) AInfernalePawn* InfernalePawn;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input") UInputMappingContext* GameInputMappingContext;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input") UInfernaleInputDataAsset* InfernaleInputDataAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input"); float TimeBeforeScrollEnd = 0.1f;

	// Draft
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Draft") bool bIsMyTurn = false;
	UPROPERTY(BlueprintReadWrite) bool bIsInDraftMode = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Settings") UGameSettingsDataAsset* GameSettingsDataAsset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug Settings") bool bPrintDebugInputMouse = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug Settings") bool bDebug = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug Settings") bool bDebugShift = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug Settings") bool bDebugSplines = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug Settings") bool bUseFluxMode = false;

	bool bIsCtrlPressed = false;
	bool bIsShiftPressed = false;
	bool bIsPPressed = false;
	bool bClickEventsAllowed = true;
	bool AllowedToReplicateThisFrame = false;
	bool AllowedToCheat = true;
	bool ReplicateUnitToHide = true;

	float CurrentTimeReplicateUnitToHide = 0.f;
	int FramesSinceLastReplicate = 0;
	float TimeReplicateUnitToHide = 0.05f;

	UPROPERTY(BlueprintReadOnly, Category = "Player Controller")
	bool bIsTacticalModeActive = false;
	
	UInputComponent* PlayerInputComponent;
	bool IsScrolling = false;
	FTimerHandle ScrollTimerHandle;
	AAmalgamVisualisationManager* AmalgamVisualisationManager;
	AUnitActorManager* UnitActorManager;
	FPlayerInfo PlayerInfo;
};