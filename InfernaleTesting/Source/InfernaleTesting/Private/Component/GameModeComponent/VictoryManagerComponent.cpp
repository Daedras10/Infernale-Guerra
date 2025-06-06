// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/GameInstanceInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Buildings/MainBuilding.h"
#include "LD/Buildings/PandemoniumMainBuilding.h"
#include "Mass/Collision/SpatialHashGrid.h"

// Sets default values for this component's properties
UVictoryManagerComponent::UVictoryManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	VictoryPoints = FVictoryPoints();
	VictoryPoints.VictoryPointsNature = 0;
	VictoryPoints.VictoryPointsTeam1 = 0;
	VictoryPoints.VictoryPointsTeam2 = 0;
	VictoryPoints.VictoryPointsTeam3 = 0;
	VictoryPoints.VictoryPointsTeam4 = 0;
	
}


// Called when the game starts
void UVictoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	GameModeInfernale->PreLaunchGame.AddDynamic(this, &UVictoryManagerComponent::OnPreLaunchGame);
	GameModeInfernale->LaunchGame.AddDynamic(this, &UVictoryManagerComponent::OnLaunchGame);
}

void UVictoryManagerComponent::OnPreLaunchGame()
{
	/* Set players present */
	//GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("OnPreLaunchGame called in VictoryManagerComponent"));
	const auto GameInstanceInfernale = Cast<UGameInstanceInfernale>(UGameplayStatics::GetGameInstance(GetWorld()));
	VictoryPoints.TeamsPresent = GameInstanceInfernale->GetPlayers();

	const auto TeamPresents = GameModeInfernale->GetPlayerStatesTeam();

	auto TeamMissing = TArray<ETeam>();
	TeamMissing.Add(ETeam::Team1);
	TeamMissing.Add(ETeam::Team2);
	TeamMissing.Add(ETeam::Team3);
	TeamMissing.Add(ETeam::Team4);
	
	for (const auto Team : TeamPresents)
	{
		TeamMissing.Remove(Team.Value);
	}

	for (const auto Team : TeamMissing)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Team %d is missing"), static_cast<int>(Team)));
		switch (Team)
		{
		case ETeam::Team1:
			VictoryPoints.VictoryPointsTeam1 = -1;
			break;
		case ETeam::Team2:
			VictoryPoints.VictoryPointsTeam2 = -1;
			break;
		case ETeam::Team3:
			VictoryPoints.VictoryPointsTeam3 = -1;
			break;
		case ETeam::Team4:
			VictoryPoints.VictoryPointsTeam4 = -1;
			break;
		default: break;
		}
	}
	
	// switch (GameInstanceInfernale->GetPlayers())
	// {
	// case 1:
	// 	VictoryPoints.VictoryPointsTeam2 = -1;
	// 	VictoryPoints.VictoryPointsTeam3 = -1;
	// 	VictoryPoints.VictoryPointsTeam4 = -1;
	// 	break;
	// case 2:
	// 	VictoryPoints.VictoryPointsTeam3 = -1;
	// 	VictoryPoints.VictoryPointsTeam4 = -1;
	// 	break;
	// case 3:
	// 	VictoryPoints.VictoryPointsTeam4 = -1;
	// 	break;
	// default: break;
	// }

	/* Set initial victory points */
	for (const auto MainBuilding : MainBuildings)
	{
		if (MainBuilding->GetOwner().Team == ETeam::NatureTeam) continue;
		AddRemoveVictoryPoints(MainBuilding->GetOwner(), MainBuilding->GetVictoryPointValue(), EVictoryPointReason::DefaultBases);
	}
}

void UVictoryManagerComponent::OnLaunchGame()
{
	//GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, TEXT("OnLaunchGame called in VictoryManagerComponent"));
	bGameStarted = true;
	StartGameTimer();
	StartPandemoniumMBTimer();
	VictoryPointChanged.Broadcast(VictoryPoints, EVictoryPointReason::DefaultBases, ETeam::NatureTeam, 0.f);
}

void UVictoryManagerComponent::OnMainBuildingCaptured(ABuildingParent* Building, const FOwner OldOwner, FOwner NewOwner)
{
	// Check if the game has started
	if (!bGameStarted) return;
	if (InitialTime - GameTimer.TimeRemaining <= 5.f) return;

	auto MainBuilding = Cast<AMainBuilding>(Building);
	const auto PandemoniumMainBuilding = Cast<APandemoniumMainBuilding>(Building);
	const bool IsPandemoniumMB = PandemoniumMainBuilding != nullptr;
	const auto GameSettings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	const bool PandemoniumMBGivesVictoryPoints = GameSettings->PandemoniumMBGivesVictoryPoints;
	if (!IsPandemoniumMB || PandemoniumMBGivesVictoryPoints)
	{
		AddRemoveVictoryPoints(OldOwner, -MainBuilding->GetVictoryPointValue(), EVictoryPointReason::MainBuildingLost);
		AddRemoveVictoryPoints(NewOwner, MainBuilding->GetVictoryPointValue(), EVictoryPointReason::MainBuildingCaptured);
	}
	
	if (!DoesPlayerStillHaveBuildings(OldOwner))
	{
		auto PlayerID = OldOwner.Player;
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Player %d has lost"), static_cast<int>(PlayerID)));
		AddKilledTeam(OldOwner.Team);
	}
}

void UVictoryManagerComponent::OnMainBuildingVictoryPointChanged(AMainBuilding* MainBuilding, const float OldValue,
	const float NewValue)
{
	AddRemoveVictoryPoints(MainBuilding->GetOwner(), NewValue - OldValue, EVictoryPointReason::MainBuildingCaptured);
}

void UVictoryManagerComponent::PandemoniumMBTimerTick(APandemoniumMainBuilding* PandemoniumMB)
{
	if (!PandemoniumMB) return;

	auto ElapsedTime = InitialTime - GameTimer.TimeRemaining;
	if (ElapsedTime < 0) ElapsedTime = 0;
	GainPandemoniumMBIncome(PandemoniumMB, ElapsedTime);
	
	const auto TimerTime = PandemoniumMB->GetVictoryPointsTimer(ElapsedTime);
	if (TimerTime <= 0) return;
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel;

	TimerDel.BindUFunction(this, FName("PandemoniumMBTimerTick"), PandemoniumMB);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, TimerTime, false);
}

void UVictoryManagerComponent::SendData()
{
	const auto Snapshots = DataGatherer->GetPlayerSnapshots();
	const auto Snapshot = Snapshots[CurrentSnapshotIndex];
	DataGatherer->ReplicatePlayerSnapshotMulticast(Snapshot);
	CurrentSnapshotIndex++;
	GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("SendData: %d"), CurrentSnapshotIndex));

	if (CurrentSnapshotIndex >= Snapshots.Num())
	{
		DataGatherer->AllInfoReplicatedMulticast(); 
		CallOnGameEnded();
		return;
	}
	
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UVictoryManagerComponent::SendData, .05f, false);
}

bool UVictoryManagerComponent::DoesPlayerStillHaveBuildings(const FOwner Owner)
{
	for (auto MainBuilding : MainBuildings)
	{
		if (MainBuilding->GetOwner().Team == Owner.Team) return true;
	}

	return false;
}

void UVictoryManagerComponent::StartGameTimer()
{
	auto GameInstanceInfernale = Cast<UGameInstanceInfernale>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GameInstanceInfernale)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("GameInstanceInfernale not found, Timer will not start"));
		return;
	}
	
	GameTimer.TimeRemaining = GameInstanceInfernale->GetGameDuration();
	InitialTime = GameTimer.TimeRemaining;
	UpdateTimer = GameTimer.TimeRemaining;
	GameDurationStarted.Broadcast(GameTimer);
	TimeRemainingDelegate.Broadcast(GameTimer);

	auto FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADataGathererActor::StaticClass());
	DataGatherer = Cast<ADataGathererActor>(FoundActor);
	if (!DataGatherer)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("DataGatherer not found"));
	}
	else
	{
		DataGatherer->SetIsActiveMulticast(true);
	}
}

void UVictoryManagerComponent::StartPandemoniumMBTimer()
{
	for (const auto PandemoniumMB : PandemoniumMainBuildings)
	{
		if (!PandemoniumMB) continue;

		const auto TimerTime = PandemoniumMB->GetVictoryPointsTimer(0);
		if (TimerTime <= 0) return;
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDel;

		TimerDel.BindUFunction(this, FName("PandemoniumMBTimerTick"), PandemoniumMB);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, TimerTime, false);
	}
}

void UVictoryManagerComponent::GainPandemoniumMBIncome(APandemoniumMainBuilding* PandemoniumMB, float TimeElapsed)
{
	if (!bGameStarted) return;

	const auto PandemoniumMBOwner = PandemoniumMB->GetOwner();
	if (PandemoniumMBOwner.Player == EPlayerOwning::Nature) return;
	if (PandemoniumMBOwner.Team == ETeam::NatureTeam) return;
	if (!DoesPlayerStillHaveBuildings(PandemoniumMBOwner)) return;
	
	AddRemoveVictoryPoints(PandemoniumMBOwner, PandemoniumMB->GetVictoryPointsPerTime(TimeElapsed), EVictoryPointReason::PandemoniumMBIncome);
	PandemoniumMB->TriggerAddRemoveVictoryPointsVFX();
}

float UVictoryManagerComponent::GetVictoryPointsOfTeam(ETeam Team) const
{
	switch (Team)
	{
	case ETeam::NatureTeam:
		return VictoryPoints.VictoryPointsNature;
	case ETeam::Team1:
		return VictoryPoints.VictoryPointsTeam1;
	case ETeam::Team2:
		return VictoryPoints.VictoryPointsTeam2;
	case ETeam::Team3:
		return VictoryPoints.VictoryPointsTeam3;
	case ETeam::Team4:
		return VictoryPoints.VictoryPointsTeam4;
	default: return -1.f;
	}
}

void UVictoryManagerComponent::CallOnVictoryPointsChanged()
{
	VictoryPointChanged.Broadcast(VictoryPoints, EVictoryPointReason::EVPNone, ETeam::NatureTeam, 0.f);
}

void UVictoryManagerComponent::CheatAddVictoryPoints(ETeam Team, float Amount, EVictoryPointReason Reason)
{
	FOwner Owner;
	Owner.Team = Team;
	Owner.Player = EPlayerOwning::Nature; /* Nature team does not have a player, but we need to set it to something */
	AddRemoveVictoryPoints(Owner, Amount, Reason);
}

void UVictoryManagerComponent::CheatEndGame()
{
	EndGameByTime();
	UpdateTimer = -1;
}

void UVictoryManagerComponent::CallOnGameEnded()
{
	OnGameEnded.Broadcast(VictoryPoints, DeadTeams, ETeam::NatureTeam);
}

void UVictoryManagerComponent::CallLaunchSequence()
{
	OnLaunchSequence.Broadcast(VictoryPoints, DeadTeams, ETeam::NatureTeam);
}

void UVictoryManagerComponent::AddRemoveVictoryPoints(const FOwner Owner, const float Value, EVictoryPointReason Reason)
{
	if (Owner.Team == ETeam::NatureTeam) return;

	if (!bGameStarted && Reason != EVictoryPointReason::DefaultBases && Reason != EVictoryPointReason::VictoryPointCheat ) return;
	AddRemoveVictoryPointsNoEvent(Owner, Value);

	// auto TeamID = GameModeInfernale->ETeamToInt(Owner.Team);
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("----------------------------------------"));
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Tryed to add %f Victory Points to %d team"), Value, TeamID));
	//
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Team %d Victory Points: %f"), 1, VictoryPoints.VictoryPointsTeam1));
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Team %d Victory Points: %f"), 2, VictoryPoints.VictoryPointsTeam2));
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Team %d Victory Points: %f"), 3, VictoryPoints.VictoryPointsTeam3));
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Team %d Victory Points: %f"), 4, VictoryPoints.VictoryPointsTeam4));
	//
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("----------------------------------------"));
	
	VictoryPointChanged.Broadcast(VictoryPoints, Reason, Owner.Team, Value);
}

void UVictoryManagerComponent::AddRemoveVictoryPointsNoEvent(const FOwner Owner, const float Value)
{
	switch (Owner.Team)
	{
	case ETeam::NatureTeam:
		if (VictoryPoints.VictoryPointsNature == -1) return;
		VictoryPoints.VictoryPointsNature += Value;
		if (VictoryPoints.VictoryPointsNature < 0) VictoryPoints.VictoryPointsNature = 0;
		break;
	case ETeam::Team1:
		if (VictoryPoints.VictoryPointsTeam1 == -1) return;
		VictoryPoints.VictoryPointsTeam1 += Value;
		//GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Team 1 Victory Points: %f"), VictoryPoints.VictoryPointsTeam1));
		if (VictoryPoints.VictoryPointsTeam1 < 0) VictoryPoints.VictoryPointsTeam1 = 0;
		break;
	case ETeam::Team2:
		if (VictoryPoints.VictoryPointsTeam2 == -1) return;
		VictoryPoints.VictoryPointsTeam2 += Value;
		if (VictoryPoints.VictoryPointsTeam2 < 0) VictoryPoints.VictoryPointsTeam2 = 0;
		break;
	case ETeam::Team3:
		if (VictoryPoints.VictoryPointsTeam3 == -1) return;
		VictoryPoints.VictoryPointsTeam3 += Value;
		if (VictoryPoints.VictoryPointsTeam3 < 0) VictoryPoints.VictoryPointsTeam3 = 0;
		break;
	case ETeam::Team4:
		if (VictoryPoints.VictoryPointsTeam4 == -1) return;
		VictoryPoints.VictoryPointsTeam4 += Value;
		if (VictoryPoints.VictoryPointsTeam4 < 0) VictoryPoints.VictoryPointsTeam4 = 0;
		break;
	}
}

void UVictoryManagerComponent::EndGameByTime()
{
	const auto Settings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	if (!Settings->EndAfterTimeDepleted) return;
	auto FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADataGathererActor::StaticClass());
	GameModeInfernale->GameCleanup();
	CallLaunchSequence();
	ASpatialHashGrid::Instance->GameEnded = true;
	DataGatherer = Cast<ADataGathererActor>(FoundActor);
	if (!DataGatherer)
	{
		OnGameEnded.Broadcast(VictoryPoints, DeadTeams, ETeam::NatureTeam);
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("DataGatherer not found"));
		CallOnGameEnded();
	}
	else
	{
		DataGatherer->SetIsActiveMulticast(false);
		SendData();
	}
}

// Called every frame
void UVictoryManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bGameStarted || bGamePaused) return;
	
	GameTimer.TimeRemaining -= DeltaTime;
	if (FMath::Floor(GameTimer.TimeRemaining) >= UpdateTimer) return;

	UpdateTimer -= 1;
	TimeRemainingDelegate.Broadcast(GameTimer);
	
	
	if (UpdateTimer == 0) EndGameByTime();
}

void UVictoryManagerComponent::AddMainBuilding(AMainBuilding* MainBuilding)
{
	MainBuildings.Add(MainBuilding);
	MainBuilding->BuildingOwnershipChanged.AddDynamic(this, &UVictoryManagerComponent::OnMainBuildingCaptured);
	MainBuilding->MainBuildingVictoryPointChanged.AddDynamic(this, &UVictoryManagerComponent::OnMainBuildingVictoryPointChanged);
}

void UVictoryManagerComponent::AddPandemoniumMainBuilding(APandemoniumMainBuilding* MainBuilding)
{
	PandemoniumMainBuildings.Add(MainBuilding);
}

FTimeRemaining UVictoryManagerComponent::GetGameTimer() const
{
	return GameTimer;
}

void UVictoryManagerComponent::SetGameTimer(float Time)
{
	if (Time < 0) return;
	GameTimer.TimeRemaining = Time;
	InitialTime = Time;
	UpdateTimer = Time;
	TimeRemainingDelegate.Broadcast(GameTimer);
}

void UVictoryManagerComponent::AddKilledTeam(ETeam Team)
{
	if (DeadTeams.Contains(Team)) return;
    DeadTeams.Add(Team);
	OnPlayerLost.Broadcast(VictoryPoints, DeadTeams, Team);
}

void UVictoryManagerComponent::AddPointsOnBossDeath(FOwner Depleter, float Value)
{
	AddRemoveVictoryPoints(Depleter, Value, EVictoryPointReason::BossKilled);
}

