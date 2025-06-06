// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Infernale/GameModeInfernale.h"

#include "Component/ActorComponents/SpawnerComponent.h"
#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameMode/GameInstanceInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Interfaces/Ownable.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Buildings/Building.h"
#include "LD/Buildings/MainBuilding.h"
#include "Manager/UnitActorManager.h"
#include "Mass/Collision/SpatialHashGrid.h"

AGameModeInfernale::AGameModeInfernale()
{
	VictoryManagerComponent = CreateDefaultSubobject<UVictoryManagerComponent>(TEXT("VictoryManagerComponent"));
	PlayerControllers = TArray<APlayerControllerInfernale*>();
}

TWeakObjectPtr<APlayerStateInfernale> AGameModeInfernale::GetPlayerState(const EPlayerOwning Player)
{
	if (PlayerStates.Num() == 0) return nullptr;
	if (PlayerStates.Contains(Player)) return PlayerStates[Player];
	
	return nullptr;
}

TArray<APlayerStateInfernale*> AGameModeInfernale::GetPlayerStates()
{
	auto PlayerStatesArray = TArray<APlayerStateInfernale*>();
	for (auto CurPlayerState : PlayerStates)
	{
		PlayerStatesArray.Add(CurPlayerState.Value);
	}
	return PlayerStatesArray;
}

void AGameModeInfernale::AddPlayerState(const FOwner NewOwner, APlayerStateInfernale* PlayerState)
{
	PlayerStates.Add(NewOwner.Player, PlayerState);
	PlayerStatesTeam.Add(NewOwner.Player, NewOwner.Team);
	PlayerState->PlayerStateOwnerChanged.AddDynamic(this, &AGameModeInfernale::OnPlayerStateOwnerChanged);
}

void AGameModeInfernale::NotifyPlayerReady(APlayerControllerInfernale* PlayerController, FPlayerInfo PlayerInfo)
{
	if (bGameStarted) return;
	auto Index = FMath::Min(PlayerInfo.PlayerID, PlayerControllers.Num());
	GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Index: %d"), Index));
	PlayerControllers.Insert(PlayerController, Index);
	
	PlayersReady++;
	PlayerReady.Broadcast(PlayerController);

	auto AllAreReady = AreAllPlayersReady();
	if (!AllAreReady) return;
	
	//Check Mode
	auto TimerHandle = FTimerHandle();
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGameModeInfernale::BeforeAllPlayersReadyWaiter, .5, false);
}

UVictoryManagerComponent* AGameModeInfernale::GetVictoryManagerComponent() const
{
	return VictoryManagerComponent;
}

AUnitActorManager* AGameModeInfernale::GetUnitActorManager() const
{
	TArray<AActor*> Actors = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), UnitActorManagerClass, Actors);
	if (Actors.Num() == 0) return nullptr;
	return Cast<AUnitActorManager>(Actors[0]);
}

TSubclassOf<AUnitActorManager> AGameModeInfernale::GetUnitActorManagerClass() const
{
	return UnitActorManagerClass;
}

float AGameModeInfernale::GetRadiusFromGameDurationAdditive() const
{
	const auto CurrentTime = VictoryManagerComponent->GetGameTimer().TimeRemaining;
	const auto Percent = (MaxDuration - CurrentTime) / MaxDuration;
	return GameDurationToRadiusCurveAdditive->GetFloatValue(Percent);
}

float AGameModeInfernale::GetRadiusFromGameDurationMultiplicative() const
{
	const auto CurrentTime = VictoryManagerComponent->GetGameTimer().TimeRemaining;
	const auto Percent = (MaxDuration - CurrentTime) / MaxDuration;
	return GameDurationToRadiusCurveMultiplicative->GetFloatValue(Percent);
}

float AGameModeInfernale::GetRadiusFromGameDuration(float InitialValue) const
{
	const auto CurrentTime = VictoryManagerComponent->GetGameTimer().TimeRemaining;
	const auto Percent = (MaxDuration - CurrentTime) / MaxDuration;
	const auto Add = GameDurationToRadiusCurveAdditive->GetFloatValue(Percent);
	const auto Mult =GameDurationToRadiusCurveMultiplicative->GetFloatValue(Percent);
	return (InitialValue * Mult) + Add;
}

void AGameModeInfernale::ResynchPS(APlayerStateInfernale* PlayerState)
{
	for (const auto CurPlayerState : PlayerStates)
	{
		if (CurPlayerState.Value != PlayerState) continue;
		const auto Info = PlayerStatesTeam[CurPlayerState.Key];
		auto NewOwner = FOwner();
		NewOwner.Player = CurPlayerState.Key;
		NewOwner.Team = Info;
		PlayerState->SetOwnerInfo(NewOwner);

		if (NewOwner.Player == EPlayerOwning::Nature) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerState with Nature player"));
		if (NewOwner.Team == ETeam::NatureTeam) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerState with Nature team"));
		
		break;
	}
}

TMap<EPlayerOwning, ETeam> AGameModeInfernale::GetPlayerStatesTeam() const
{
	return PlayerStatesTeam;
}

TArray<APlayerControllerInfernale*> AGameModeInfernale::GetPlayerControllers() const
{
	return PlayerControllers;
}

bool AGameModeInfernale::IsGameStarted() const
{
	return bGameStarted;
}

void AGameModeInfernale::BeforeAllPlayersReadyWaiter()
{
	AllPlayersSpawned.Broadcast();
	
	const auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());
	if (!GameInstance) return;

	/* Set all players to their owner and register infos */
	SetAllPlayerState();

	
	switch (GameInstance->GetCustomGameMode())
	{
	case 0:
		RandomDraftBasesForPlayers();
		BeforeAllPlayersReady();
		break;
	case 3:
		StartDraft();
		BeforeAllPlayersReady();
		break;
	default:
		MoveAllPlayersToSpawns();
		AllBasesAssigned.Broadcast();
		BeforeAllPlayersReady();
		break;
	}
	
}

void AGameModeInfernale::RandomDraftBasesForPlayers()
{
	const auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());
	if (!GameInstance) return;
	GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Purple, TEXT("DraftBasesForPlayers"));
	
	TArray<AMainBuilding*> MainBuildings;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), StarterMainBuildingTag, Actors);
	for (auto Actor : Actors)
	{
		auto MainBuilding = Cast<AMainBuilding>(Actor);
		if (!MainBuilding) continue;
		MainBuildings.Add(MainBuilding);
	}

	/* MainBuildings with default owners */
	TArray<AActor*> AllMainBuildingsAsActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainBuilding::StaticClass(), AllMainBuildingsAsActors);

	/* Clear Teams */
	FOwner Nature = FOwner();
	Nature.Player = EPlayerOwning::Nature;
	Nature.Team = ETeam::NatureTeam;
	for (auto MainBuilding : MainBuildings)
	{
		MainBuilding->ChangeOwner(Nature);
	}
	

	/* Draft Bases to Teams */
	const int MaxIndex = MainBuildings.Num() - 1;
	const int NumberOfPlayers = GameInstance->GetPlayers();
	const int CustomTeamMode = GameInstance->GetCustomTeamMode();

	TArray<ETeam> TeamsPresentOtherMods = TArray<ETeam>();
	int PlayersAttributedT1 = 0;
	int PlayersAttributedT2 = 0;
	if (CustomTeamMode == 0 || CustomTeamMode == 2) /* 2v2 or 1v1 */
	{
		for (auto PlayerStateTeam : PlayerStatesTeam)
		{
			TeamsPresentOtherMods.AddUnique(PlayerStateTeam.Value);
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Purple, FString::Printf(TEXT("TeamsPresentOtherMods: %d"), TeamsPresentOtherMods.Num()));

	
	for (int PlayerIndex = 0; PlayerIndex < NumberOfPlayers; PlayerIndex++)
	{
		const auto Player = PlayerControllers[PlayerIndex];
		const auto Pawn = Player->GetInfernalePawn();
		if (!Pawn)
		{
			GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Pawn not found in AGameModeInfernale::DraftBasesForPlayers"));
			continue;
		}
		int BasesAllocated = 0;
		AMainBuilding* MainBuilding = nullptr;
		
		for (const auto Actor : AllMainBuildingsAsActors)
		{
			const auto MB = Cast<AMainBuilding>(Actor);
			if (!MB) continue;
			if (!MB->StartOwned()) continue;
			const auto OldOwner = MB->GetOwner();
			APlayerStateInfernale* PlayerState = Player->GetPlayerState<APlayerStateInfernale>();
			if (OldOwner.Team != ETeam::NatureTeam) continue;
			int PlayerAsID = EPlayerToInt(OldOwner.Player);
			const auto OwnerInfo = PlayerState->GetOwnerInfo();
			if (!PlayerState) continue;
			const auto NewOwner = PlayerState->GetOwnerInfo();

			if (CustomTeamMode == 1)
			{
				if (ETeamToInt(OwnerInfo.Team) != PlayerAsID) continue;
				
				MB->ChangeOwner(NewOwner);
				const auto CameraProfileLocation = MB->GetActorLocation() + FVector(-5000.0f, 0, Pawn->GetActorLocation().Z);
				const auto CameraProfileRotation = FRotator(-65, 0, 0);
				Pawn->AddOverrideCameraProfileIndexPosition(0, CameraProfileLocation, CameraProfileRotation);
				BasesAllocated++;
			}
			else if (CustomTeamMode == 0) /* 2v2 */
			{
				bool IsFirstTeam = OwnerInfo.Team == TeamsPresentOtherMods[0];
				if (IsFirstTeam)
				{
					if (PlayersAttributedT1 == 0 && OldOwner.Player == EPlayerOwning::Player1 ||
						PlayersAttributedT1 == 1 && OldOwner.Player == EPlayerOwning::Player4)
					{
						MB->ChangeOwner(NewOwner);
						const auto CameraProfileLocation = MB->GetActorLocation() + FVector(-5000.0f, 0, Pawn->GetActorLocation().Z);
						const auto CameraProfileRotation = FRotator(-65, 0, 0);
						Pawn->AddOverrideCameraProfileIndexPosition(0, CameraProfileLocation, CameraProfileRotation);
						BasesAllocated++;
						PlayersAttributedT1++;
						break;
					}
				}
				else
				{
					if (PlayersAttributedT2 == 0 && OldOwner.Player == EPlayerOwning::Player2 ||
						PlayersAttributedT2 == 1 && OldOwner.Player == EPlayerOwning::Player3)
					{
						MB->ChangeOwner(NewOwner);
						const auto CameraProfileLocation = MB->GetActorLocation() + FVector(-5000.0f, 0, Pawn->GetActorLocation().Z);
						const auto CameraProfileRotation = FRotator(-65, 0, 0);
						Pawn->AddOverrideCameraProfileIndexPosition(0, CameraProfileLocation, CameraProfileRotation);
						BasesAllocated++;
						PlayersAttributedT2++;
						break;
					}
				}
			}
			else if (CustomTeamMode == 2) /* 1v1 */
			{
				if (PlayerAsID == 1 && OwnerInfo.Player == EPlayerOwning::Player1
					|| PlayerAsID == 2 && OwnerInfo.Player == EPlayerOwning::Player2)
				{
					MB->ChangeOwner(NewOwner);
					const auto CameraProfileLocation = MB->GetActorLocation() + FVector(-5000.0f, 0, Pawn->GetActorLocation().Z);
					const auto CameraProfileRotation = FRotator(-65, 0, 0);
					Pawn->AddOverrideCameraProfileIndexPosition(0, CameraProfileLocation, CameraProfileRotation);
					BasesAllocated++;
				}
			}
			
			
		}
		
		while (BasesAllocated < DraftNumberOfBases)
		{
			const int RandomIndex = FMath::RandRange(0, MaxIndex);
			if (RandomIndex >= MainBuildings.Num()) break;
			MainBuilding = MainBuildings[RandomIndex];
			const auto OldOwner = MainBuilding->GetOwner();
			if (OldOwner.Player != EPlayerOwning::Nature) continue;
			if (OldOwner.Team != ETeam::NatureTeam) continue;
			const FOwner NewOwner = PlayerControllers[PlayerIndex]->GetPlayerInfo().PlayerOwnerInfo;
				//GetOwnerInfoPlayer(PlayerIndex);
		
			if (MainBuilding->StartOwned()) continue;
		
			MainBuilding->ChangeOwner(NewOwner);
			auto CameraProfileLocation = MainBuilding->GetActorLocation() + FVector(-5000.0f, 0, Pawn->GetActorLocation().Z);
			auto CameraProfileRotation = FRotator(-65, 0, 0);
			Pawn->AddOverrideCameraProfileIndexPosition(BasesAllocated, CameraProfileLocation, CameraProfileRotation);
			BasesAllocated++;
		}
		
		Pawn->SetCanMoveOwning(false);
		Pawn->MoveToSpawn(Pawn->GetPawnStartLocation());
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Player %d has %d bases"), PlayerIndex, BasesAllocated));
	}

	for (auto Actor : AllMainBuildingsAsActors)
	{
		auto MB = Cast<AMainBuilding>(Actor);
		if (!MB) continue;
		if (!MB->StartOwned()) continue;
		const auto OldOwner = MB->GetOwner();
		FOwner NewOwner;
		NewOwner.Player = EPlayerOwning::Nature;
		NewOwner.Team = ETeam::NatureTeam;
		if (OldOwner.Team != ETeam::NatureTeam) continue;
		MB->ChangeOwner(NewOwner);
	}
	
	AllBasesAssigned.Broadcast();
}

void AGameModeInfernale::BeginPlay()
{
	Super::BeginPlay();
	VictoryManagerComponent->GameDurationStarted.AddDynamic(this, &AGameModeInfernale::OnGameDurationStarted);
}

void AGameModeInfernale::OnPlayerStateOwnerChanged(APlayerStateInfernale* PlayerState, const FOwner NewOwner)
{
	for (auto CurPlayerState : PlayerStates)
	{
		if (CurPlayerState.Value != PlayerState) continue;
		PlayerStates.Remove(CurPlayerState.Key);
		PlayerStatesTeam.Remove(CurPlayerState.Key);
		break;
	}
	PlayerStates.Add(NewOwner.Player, PlayerState);
	PlayerStatesTeam.Add(NewOwner.Player, NewOwner.Team);
}

void AGameModeInfernale::OnGameDurationStarted(FTimeRemaining GameDuration)
{
	MaxDuration = GameDuration.TimeRemaining;
	GetWorld()->GetTimerManager().SetTimer(RadiusChangeTimerHandle, this, &AGameModeInfernale::UpdateRadiusChange, RadiusChangeUpdate, true);
}

void AGameModeInfernale::StartDraft()
{
	if (PlayerControllers.Num() == 0) return;
	Cast<APlayerControllerInfernale>(PlayerControllers[CurrentPlayerTurn])->StartTurnOwning();
	for (auto PlayerController : PlayerControllers)
	{
		const auto Pawn = PlayerController->GetInfernalePawn();
		Pawn->MoveToSpawn(FVector(0,0,10000));
		PlayerController->SetDraftMode(true);
	}
}



void AGameModeInfernale::CycleTurns()
{
	Cast<APlayerControllerInfernale>(PlayerControllers[CurrentPlayerTurn])->EndTurnOwning();
	CurrentPlayerTurn++;
	CurrentPlayerTurn %= PlayerControllers.Num();
	if (CurrentPlayerTurn == 0)
	{
		CurrentCycle++;
		
		for (auto PlayerController : PlayerControllers)
		{
			PlayerController->EndCycleOwning();
		}
		if (CurrentCycle == MaxCycles)
		{
			for (auto PlayerController : PlayerControllers)
			{
				PlayerController->EndDraftOwning();
				PlayerController->SetDraftMode(false);
			}
			AllBasesAssigned.Broadcast();
			LaunchGameAfterDelay(false, true);
			return;
		}
	}
	Cast<APlayerControllerInfernale>(PlayerControllers[CurrentPlayerTurn])->StartTurnOwning();
}

void AGameModeInfernale::GameCleanup()
{
	GameEnded = true;
	KillEverything();
	TArray<AActor*> BuildingActors = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), BuildingActors);
	for (auto BuildingActor : BuildingActors)
	{
		auto Building = Cast<ABuilding>(BuildingActor);
		if (!Building) continue;
		auto SpawnerComponent = Building->GetSpawnerComponent();
		if (!SpawnerComponent) continue;
		SpawnerComponent->DisableSpawning();
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("SpawnerComponent disabled"));
	}

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGameModeInfernale::KillEverything, 1.f, false);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGameModeInfernale::KillEverything, 2.f, false);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGameModeInfernale::KillEverything, 3.f, false);
}

void AGameModeInfernale::MoveAllPlayersToSpawns()
{
	int Index;
	auto PlayerStarts = TArray<APlayerStart*>();
	auto Actors = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), Actors);
	for (auto Actor : Actors)
	{
		auto PlayerStart = Cast<APlayerStart>(Actor);
		if (!PlayerStart) continue;
		PlayerStarts.Add(PlayerStart);
	}
	auto PlayerStartsNum = PlayerStarts.Num();
	if (PlayerStartsNum == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("No PlayerStarts found in AGameModeInfernale::MoveAllPlayersToSpawns"));
		return;
	}
	auto PlayerStartsOrdered = TArray<APlayerStart*>();

	bool Found = true;
	FString TagToFind = "PlayerStart";
	Index = 0;
	while (Found)
	{
		Found = false;
		for (auto PlayerStart : PlayerStarts)
		{
			TagToFind = "PlayerStart" + FString::FromInt(Index);
			auto PSTags = PlayerStart->Tags;
			for (auto Tag : PSTags)
			{
				if (Tag != TagToFind) continue;
				PlayerStartsOrdered.Add(PlayerStart);
				Found = true;
				break;
			}
		}
		Index++;
	}

	Index = 0;
	PlayerStartsNum = PlayerStartsOrdered.Num();
	if (PlayerStartsNum == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("No PlayerStarts found in AGameModeInfernale::MoveAllPlayersToSpawns"));
		return;
	}
	for (const auto PlayerController : PlayerControllers)
	{
		const auto PlayerStart = PlayerStartsOrdered[FMath::Min(Index, PlayerStartsNum-1)];
		const auto Location = PlayerStart->GetActorLocation();
		PlayerController->MoveToSpawn(Location);
		Index++;
	}
}

void AGameModeInfernale::UpdateRadiusChange()
{
	MaxRadiusChanged.Broadcast();
}

void AGameModeInfernale::BeforeAllPlayersReady()
{
	//bGameStarted = true;
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGameModeInfernale::PostSetupStartGame, 0.5f, false);
}

void AGameModeInfernale::SetAllPlayerState()
{
	int Index = 0;
	auto Players = TArray<EPlayerOwning>();
	auto PlayerNames = TArray<FString>();
	
	for (const auto PlayerController : PlayerControllers)
	{
		APlayerStateInfernale* PlayerState = PlayerController->GetPlayerState<APlayerStateInfernale>();
		if (!PlayerState)
		{
			GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerState not found in AGameModeInfernale::SetAllPlayerState"));
			Index++;
			continue;
		}

		const auto PlayerInfo = PlayerController->GetPlayerInfo();
		const FOwner OwnerInfo = PlayerInfo.PlayerOwnerInfo;
		PlayerState->Init(OwnerInfo);
		AddPlayerState(OwnerInfo, PlayerState);

		Players.Add(OwnerInfo.Player);
		PlayerNames.Add(PlayerInfo.PlayerName);
		
		Index++;
	}

	for (const auto PlayerController : PlayerControllers)
	{
		APlayerStateInfernale* PlayerState = PlayerController->GetPlayerState<APlayerStateInfernale>();
		PlayerState->SetPlayerNamesServer(Players, PlayerNames);
	}
}

void AGameModeInfernale::PostSetupStartGame()
{
	AllPlayersReady.Broadcast();

	
	const auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());
	if (!GameInstance) return;
	switch (GameInstance->GetCustomGameMode())
	{
	case 3:
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("Draft GameModeInfernale"));
		LaunchGameAfterDelay(true,false);
		for (auto PlayerController : PlayerControllers)
		{
			PlayerController->RemoveLoadingScreen();
		}
		break;
	default:
		LaunchGameAfterDelay(true,true);
		break;
	}
}

bool AGameModeInfernale::AreAllPlayersReady() const
{
	auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());
	GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, FString::Printf(TEXT("PlayersReady: %d / %d"), PlayersReady, GameInstance->GetPlayers()));
	return PlayersReady == GameInstance->GetPlayers();
}

void AGameModeInfernale::LaunchGameAfterDelay(bool bPrelaunchGame, bool bLaunchGame)
{
	if (bPrelaunchGame)
	{
		FTimerHandle TimerHandlePreLaunchGame;
		GetWorld()->GetTimerManager().SetTimer(TimerHandlePreLaunchGame, this, &AGameModeInfernale::BroadcastPreLaunchGame, PreLaunchGameDelay, false);
	}
	if (bLaunchGame)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGameModeInfernale::BroadcastLaunchGame, LaunchGameDelay, false);
	}
}

void AGameModeInfernale::BroadcastPreLaunchGame()
{
	PreLaunchGame.Broadcast();
}

void AGameModeInfernale::BroadcastLaunchGame()
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Broadcasting LaunchGame"));
	bGameStarted = true;
	LaunchGame.Broadcast();
	
	for (auto PlayerController : PlayerControllers)
	{
		PlayerController->RemoveLoadingScreen();
		Cast<AInfernalePawn>(PlayerController->GetPawn())->SetDoCameraStartGameTravelingOwning(true);
	}
}

FOwner AGameModeInfernale::GetOwnerInfoPlayer(int Index)
{
	FOwner OwnerInfo;

	// Should depend on gamemode settings
	switch (Index)
	{
		case 0:
			OwnerInfo.Player = EPlayerOwning::Player1;
			OwnerInfo.Team = ETeam::Team1;
			break;
		case 1:
			OwnerInfo.Player = EPlayerOwning::Player2;
			OwnerInfo.Team = ETeam::Team2;
			break;
		case 2:
			OwnerInfo.Player = EPlayerOwning::Player3;
			OwnerInfo.Team = ETeam::Team3;
			break;
	case 3:
			OwnerInfo.Player = EPlayerOwning::Player4;
			OwnerInfo.Team = ETeam::Team4;
			break;
		default:
			GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Player index not found in AGameModeInfernale::GetOwnerInfoPlayer"));
			break;
	}

	return OwnerInfo;
}

FString AGameModeInfernale::GetPlayerNameInfo(int Index)
{
	const auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());
	if (!GameInstance) return FString();

	auto Personal = GameInstance->PersonalPlayerInfo;
	auto OtherPlayers = GameInstance->OpponentPlayerInfo;

	if (Index == Personal.PlayerID) return Personal.PlayerName;
	
	for (auto PlayerInfo : OtherPlayers)
	{
		if (PlayerInfo.PlayerID != Index) continue;
		return PlayerInfo.PlayerName;
	}
	
	return FString();
}

int AGameModeInfernale::EPlayerToInt(EPlayerOwning Player)
{
	switch (Player)
	{
	case EPlayerOwning::Nature:
		return 0;
	case EPlayerOwning::Player1:
		return 1;
	case EPlayerOwning::Player2:
		return 2;
	case EPlayerOwning::Player3:
		return 3;
	case EPlayerOwning::Player4:
		return 4;
	default:
		return 0;
	}
}

int AGameModeInfernale::ETeamToInt(ETeam Team)
{
	switch (Team)
	{
	case ETeam::NatureTeam:
		return 0;
	case ETeam::Team1:
		return 1;
	case ETeam::Team2:
		return 2;
	case ETeam::Team3:
		return 3;
	case ETeam::Team4:
		return 4;
	default:
		return 0;
	}
}

EPlayerOwning AGameModeInfernale::IntToEPlayer(int Index)
{
	switch (Index)
	{
	case 0:
		return EPlayerOwning::Nature;
	case 1:
		return EPlayerOwning::Player1;
	case 2:
		return EPlayerOwning::Player2;
	case 3:
		return EPlayerOwning::Player3;
	case 4:
		return EPlayerOwning::Player4;
	default:
		return EPlayerOwning::Nature;
	}
}

ETeam AGameModeInfernale::IntToETeam(int Index)
{
	switch (Index)
	{
	case 0:
		return ETeam::NatureTeam;
	case 1:
		return ETeam::Team1;
	case 2:
		return ETeam::Team2;
	case 3:
		return ETeam::Team3;
	case 4:
		return ETeam::Team4;
	default:
		return ETeam::NatureTeam;
	}
}

void AGameModeInfernale::StartNewGame(float NewTimeLeft)
{
	const auto GameInstance = Cast<UGameInstanceInfernale>(GetGameInstance());
	GameInstance->SetGameDuration(NewTimeLeft);

	const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	const auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
	if (!PlayerControllerInfernale) return;
	PlayerControllerInfernale->LaunchNewGameBP();
}

bool AGameModeInfernale::GameHasEnded() const
{
	return GameEnded;
}

void AGameModeInfernale::CheatAddSoulsToPlayer(int PlayerID, float Amount)
{
	EPlayerOwning Player = IntToEPlayer(PlayerID);
	if (Player == EPlayerOwning::Nature) return;
	if (!PlayerStates.Contains(Player)) return;

	const auto PS = GetPlayerState(Player);
	if (!PS.IsValid()) return;

	PS->GetEconomyComponent()->GainCheatIncome(Amount);
}

void AGameModeInfernale::KillEverything()
{
	TArray<FMassEntityHandle> Keys = ASpatialHashGrid::GetAllEntityHandles();
	if (Keys.Num() == 0) return;
	
	for (auto Key : Keys)
	{
		const auto Data = ASpatialHashGrid::GetMutableEntityData(Key);
		if (!Data) continue;
		Data->DamageEntity(99999999.f);
	}
}
