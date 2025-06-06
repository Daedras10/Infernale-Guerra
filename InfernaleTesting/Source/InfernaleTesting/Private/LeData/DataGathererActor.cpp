// Fill out your copyright notice in the Description page of Project Settings.


#include "LeData/DataGathererActor.h"

#include "JsonObjectConverter.h"
#include "VectorTypes.h"
#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "LD/LDElement/Boss.h"
#include "LD/LDElement/NeutralCamp.h"
#include "Mass/Collision/SpatialHashGrid.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ADataGathererActor::ADataGathererActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ADataGathererActor::BeginPlay()
{
	Super::BeginPlay();

	//get all the actor of class AMainBuilding in the map
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMainBuilding::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		AMainBuilding* MainBuilding = Cast<AMainBuilding>(Actor);
		if (MainBuilding)
		{
			MainBuildings.Add(MainBuilding);
		}
	}

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABoss::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		if (ABoss* Boss = Cast<ABoss>(Actor))
		{
			Bosses.Add(Boss);
		}
	}

	TotalSoulsGainForLastSnapshot.Add(ETeam::Team1, 0.f);
	TotalSoulsGainForLastSnapshot.Add(ETeam::Team2, 0.f);
	TotalSoulsGainForLastSnapshot.Add(ETeam::Team3, 0.f);
	TotalSoulsGainForLastSnapshot.Add(ETeam::Team4, 0.f);

	AveragedTotalSoulsGainForLastSnapshot.Add(ETeam::Team1, 0.f);
	AveragedTotalSoulsGainForLastSnapshot.Add(ETeam::Team2, 0.f);
	AveragedTotalSoulsGainForLastSnapshot.Add(ETeam::Team3, 0.f);
	AveragedTotalSoulsGainForLastSnapshot.Add(ETeam::Team4, 0.f);

	EarnedSoulsFromMonsters.Add(ETeam::Team1, 0.f);
	EarnedSoulsFromMonsters.Add(ETeam::Team2, 0.f);
	EarnedSoulsFromMonsters.Add(ETeam::Team3, 0.f);
	EarnedSoulsFromMonsters.Add(ETeam::Team4, 0.f);

	AverageSoulsGain.Add(ETeam::Team1, 0.f);
	AverageSoulsGain.Add(ETeam::Team2, 0.f);
	AverageSoulsGain.Add(ETeam::Team3, 0.f);
	AverageSoulsGain.Add(ETeam::Team4, 0.f);

	SnapshotsSoulsGain.Add(ETeam::Team1, TArray<float>());
	SnapshotsSoulsGain.Add(ETeam::Team2, TArray<float>());
	SnapshotsSoulsGain.Add(ETeam::Team3, TArray<float>());
	SnapshotsSoulsGain.Add(ETeam::Team4, TArray<float>());

	
}

// Called every frame
void ADataGathererActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bActive)
	{
		TimeSinceLastSnapshot += DeltaTime;
		if (TimeSinceLastSnapshot >= TimeBetweenSnapshotsInSeconds)
		{
			TimeSinceLastSnapshot = 0.f;
			TakeSnapshot();

			FTotalSoulsGainForLastSnapshotStruct TotalSoulsGainForLastSnapshotStruct;
			for (auto& [Team, Amount] : TotalSoulsGainForLastSnapshot)
			{
				FFloatTeamStruct FloatTeamStruct;
				FloatTeamStruct.Team = Team;
				FloatTeamStruct.Value = Amount;
				TotalSoulsGainForLastSnapshotStruct.TotalSoulsGainForLastSnapshot.Add(FloatTeamStruct);
			}
			for (auto& [Team, Amount] : AveragedTotalSoulsGainForLastSnapshot)
			{
				FFloatTeamStruct FloatTeamStruct;
				FloatTeamStruct.Team = Team;
				FloatTeamStruct.Value = Amount;
				TotalSoulsGainForLastSnapshotStruct.AveragedTotalSoulsGainForLastSnapshot.Add(FloatTeamStruct);
			}
			ReplicateTotalSoulsGainForLastSnapshotMulticast(TotalSoulsGainForLastSnapshotStruct);
		}
	}
}

void ADataGathererActor::SetIsActiveMulticast_Implementation(bool bIsActive)
{
	bActive = bIsActive;
}

void ADataGathererActor::ReplicatePlayerSnapshotMulticast_Implementation(const FPlayerSnapshot& PlayerSnapshotRep)
{
	if (HasAuthority()) return;
	PlayerSnapshots.Add(PlayerSnapshotRep);
}

void ADataGathererActor::AllInfoReplicatedMulticast_Implementation()
{
	JsonSavePath = FPaths::ProjectSavedDir() + "SavedGameData/" + "PlayerSnapshots.json";
	SaveSnapshotsToJson();
}

bool ADataGathererActor::GatherEconomyDataForPlayer(APlayerStateInfernale* PlayerState,
                                                    FDataGathererPlayerInfo& PlayerInfo, bool& bValue)
{
	if (!PlayerState) return true;
	auto EconomyComponent = PlayerState->GetEconomyComponent();
	TWeakObjectPtr<UEconomyComponent> EconomyComponentWeak = EconomyComponent;
	if (!EconomyComponent || !EconomyComponentWeak.IsValid())
	{
		bValue = true;
		return true;
	}
	PlayerInfo.SoulsInReserve = EconomyComponent->GetSouls();
	if (EarnedSoulsFromMonsters.Contains(PlayerInfo.Owner.Team))
	{
		PlayerInfo.SoulsFromMonsters = EarnedSoulsFromMonsters[PlayerInfo.Owner.Team];
		EarnedSoulsFromMonsters[PlayerInfo.Owner.Team] = 0.f;
	}
	else
	{
		PlayerInfo.SoulsFromMonsters = 0;
		UE_LOG(LogTemp, Warning, TEXT("Team not found in EarnedSoulsFromMonsters: %d"),
		       static_cast<uint8>(PlayerInfo.Owner.Team));
	}
	PlayerInfo.SoulsFromIncome = EconomyComponent->BuildingIncome;
	PlayerInfo.SoulsFromBeacons = EconomyComponent->SoulsBeaconIncome;

	EconomyComponent->BuildingIncome = 0.f;
	EconomyComponent->SoulsBeaconIncome = 0.f;

	if (TotalSoulsGainForLastSnapshot.Contains(PlayerInfo.Owner.Team))
	{
		TotalSoulsGainForLastSnapshot[PlayerInfo.Owner.Team] = PlayerInfo.SoulsFromMonsters + PlayerInfo.SoulsFromIncome
			+ PlayerInfo.SoulsFromBeacons;
	}
	if (AveragedTotalSoulsGainForLastSnapshot.Contains(PlayerInfo.Owner.Team))
	{
		if (SnapshotsSoulsGain.Contains(PlayerInfo.Owner.Team))
		{
			if (TotalSoulsGainForLastSnapshot.Contains(PlayerInfo.Owner.Team) && SnapshotsSoulsGain.Contains(
				PlayerInfo.Owner.Team))
				SnapshotsSoulsGain[PlayerInfo.Owner.Team].Insert(TotalSoulsGainForLastSnapshot[PlayerInfo.Owner.Team],
				                                                 0);
		}
		else
		{
			SnapshotsSoulsGain.Add(PlayerInfo.Owner.Team, TArray<float>());
		}

		if (SnapshotsSoulsGain[PlayerInfo.Owner.Team].Num() > NumberOfSnapshotsPerAverageSoulsGain)
		{
			SnapshotsSoulsGain[PlayerInfo.Owner.Team].RemoveAt(NumberOfSnapshotsPerAverageSoulsGain, 1);
		}
		float TotalSoulsGain = 0.f;
		for (auto& Amount : SnapshotsSoulsGain[PlayerInfo.Owner.Team])
		{
			TotalSoulsGain += Amount;
		}
		AveragedTotalSoulsGainForLastSnapshot[PlayerInfo.Owner.Team] = TotalSoulsGain / SnapshotsSoulsGain[PlayerInfo.
		                                                                                                   Owner.Team].
			Num() / TimeBetweenSnapshotsInSeconds;
	}

	return false;
}

bool ADataGathererActor::GatherPlayerInfoSnapshot(FPlayerSnapshot* Snapshot,
                                                  UVictoryManagerComponent* VictoryManagerComponent,
                                                  APlayerStateInfernale* PlayerState)
{
	if (PlayerState == nullptr) return true;
	FDataGathererPlayerInfo PlayerInfo;
	PlayerInfo.Owner = PlayerState->GetOwnerInfo();
	bool bValue;
	if (bGatherEconomy)
	{
		if (GatherEconomyDataForPlayer(PlayerState, PlayerInfo, bValue)) return bValue;
	}
	if (bGatherBosses)
	{
		if (BossesKilled.Contains(PlayerInfo.Owner.Team))
		{
			PlayerInfo.BossesKilled = BossesKilled[PlayerInfo.Owner.Team];
			BossesKilled[PlayerInfo.Owner.Team] = 0.f;
		}
		else
		{
			PlayerInfo.BossesKilled = 0;
			/* UE_LOG(LogTemp, Warning, TEXT("Team not found in BossesKilled: %d"), static_cast<uint8>(PlayerInfo.Owner.Team)); */
		}
	}
	if (bGatherDominationPoints)
	{
		PlayerInfo.DominationPoints = VictoryManagerComponent->GetVictoryPointsOfTeam(PlayerInfo.Owner.Team);
	}
	if (bGatherTransmutationInfos)
	{
		const auto PC = Cast<APlayerControllerInfernale>(PlayerState->GetPlayerController());
		if (PC)
		{
			const auto TC = PC->GetTransmutationComponent();
			if (TC) PlayerInfo.ActiveTransmutation = TC->GetActiveTransmutations();
		}
	}

	//count the number of bases for this player
	if (bGatherMainBuilding)
	{
		int BasesCount = 0;
		for (auto MainBuilding : MainBuildings)
		{
			if (!MainBuilding) continue;
			auto BuildingOwner = MainBuilding->GetOwner();
			if (BuildingOwner.Player == PlayerInfo.Owner.Player && BuildingOwner.Team == PlayerInfo.Owner.Team)
			{
				BasesCount++;
			}
		}
		PlayerInfo.NumberOfBases = BasesCount;
	}

	if (bGatherAmalgams)
	{
		if (bGatherAmalgamPositions)
		{
			FAmalgamPresence Amalgam;
			Amalgam.Team = PlayerInfo.Owner.Team;
			Amalgam.Type = "Behemot";
			TArray<FVector2DInt> ToVector2DInt;
			for (const auto& Behemot : ASpatialHashGrid::GetAllEntityOfTypeOfTeam(
				EEntityType::EntityTypeBehemot, PlayerInfo.Owner.Team))
			{
				ToVector2DInt.Add(FVector2DInt(Behemot));
			}
			Amalgam.Positions = ToVector2DInt;
			Snapshot->AmalgamPositionsAndType.Add(Amalgam);

			Amalgam.Team = PlayerInfo.Owner.Team;
			Amalgam.Type = "Gobborit";
			ToVector2DInt.Empty();
			for (const auto& Gobborit : ASpatialHashGrid::GetAllEntityOfTypeOfTeam(
				EEntityType::EntityTypeGobborit, PlayerInfo.Owner.Team))
			{
				ToVector2DInt.Add(FVector2DInt(Gobborit));
			}
			Amalgam.Positions = ToVector2DInt;
			Snapshot->AmalgamPositionsAndType.Add(Amalgam);
			
			Amalgam.Team = PlayerInfo.Owner.Team;
			Amalgam.Type = "Nerras";
			ToVector2DInt.Empty();
			for (const auto& Nerras : ASpatialHashGrid::GetAllEntityOfTypeOfTeam(
				EEntityType::EntityTypeNerras, PlayerInfo.Owner.Team))
			{
				ToVector2DInt.Add(FVector2DInt(Nerras));
			}
			Amalgam.Positions = ToVector2DInt;
			Snapshot->AmalgamPositionsAndType.Add(Amalgam);
			ToVector2DInt.Empty();
		}
	}

	Snapshot->PlayerInfos.Add(PlayerInfo);
	return false;
}

float ADataGathererActor::GetAverageSoulsGainOfTeam(ETeam Team)
{
	if (!AveragedTotalSoulsGainForLastSnapshot.Contains(Team))
	{
		UE_LOG(LogTemp, Warning, TEXT("Team not found in AverageSoulsGain: %d"), static_cast<uint8>(Team));
		return -1.f;
	}
	return AveragedTotalSoulsGainForLastSnapshot[Team];
}

void ADataGathererActor::ReplicateTotalSoulsGainForLastSnapshotMulticast_Implementation(
	FTotalSoulsGainForLastSnapshotStruct ReplicatedStruct)
{
	for (auto& [Team, Amount] : ReplicatedStruct.TotalSoulsGainForLastSnapshot)
	{
		if (TotalSoulsGainForLastSnapshot.Contains(Team))
		{
			TotalSoulsGainForLastSnapshot[Team] = Amount;
		}
		else
		{
			TotalSoulsGainForLastSnapshot.Add(Team, Amount);
		}
	}
	for (auto& [Team, Amount] : ReplicatedStruct.AveragedTotalSoulsGainForLastSnapshot)
	{
		if (AveragedTotalSoulsGainForLastSnapshot.Contains(Team))
		{
			AveragedTotalSoulsGainForLastSnapshot[Team] = Amount;
		}
		else
		{
			AveragedTotalSoulsGainForLastSnapshot.Add(Team, Amount);
		}
	}
}

void ADataGathererActor::TakeSnapshot()
{
	if (!HasAuthority()) return;
	FPlayerSnapshot Snapshot;
	auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return;
	auto VictoryManagerComponent = GameMode->GetVictoryManagerComponent();
	Snapshot.Time = GameMode->GetVictoryManagerComponent()->GetGameTimer().TimeRemaining;
	Snapshot.TimeMMSS = Snapshot.GetFormattedTime();
	auto PlayerStates = GameMode->GetPlayerStates();
	if (bGatherPlayerInfo)
	{
		for (auto PlayerState : PlayerStates)
		{
			if (GatherPlayerInfoSnapshot(&Snapshot, VictoryManagerComponent, PlayerState))
			{
				UE_LOG(LogTemp, Error, TEXT("Error in GatherPlayerInfoSnapshot"));
			}
		}
	}

	if (bGatherAmalgams)
	{
		Snapshot.AmalgamsOnMap = ASpatialHashGrid::GetEntitiesCount();
	}

	if (bGatherMainBuilding)
	{
		for (AMainBuilding* Building : MainBuildings)
		{
			FBaseCaptureInfo CaptureInfo;
			CaptureInfo.Position.X = Building->GetActorLocation().X;
			CaptureInfo.Position.Y = Building->GetActorLocation().Y;
			CaptureInfo.Name = Building->GetBuildingName();
			CaptureInfo.Team = Building->GetOwner().Team;
			Snapshot.BasesInfos.Add(CaptureInfo);
		}
	}

	if (bGatherBosses)
	{
		for (ABoss* Boss : Bosses)
		{
			if (!Boss) continue;
			FBossKillInfo BossKillInfo;
			BossKillInfo.Position.X = Boss->GetActorLocation().X;
			BossKillInfo.Position.Y = Boss->GetActorLocation().Y;
			BossKillInfo.CaptureTeam = Boss->GetOwner().Team;
			Snapshot.BossesKilledInfo.Add(BossKillInfo);
		}
	}

	PlayerSnapshots.Add(Snapshot);
}

void ADataGathererActor::AddMonsterSoulsToOwner(FOwner Player, float Amount, ECampTypeForData CampType)
{
	ETeam Team = Player.Team;
	if (EarnedSoulsFromMonsters.Contains(Team))
	{
		EarnedSoulsFromMonsters[Team] += Amount;
	}
	else
	{
		EarnedSoulsFromMonsters.Add(Team, Amount);
	}
	switch (CampType)
	{
	case ECampTypeForData::NeutralCamp:
		if (SmallCampsKilled.Contains(Team))
		{
			SmallCampsKilled[Team]++;
		}
		else
		{
			SmallCampsKilled.Add(Team, 1);
		}
		break;
	case ECampTypeForData::BigNeutralCamp:
		if (BigCampsKilled.Contains(Team))
		{
			BigCampsKilled[Team]++;
		}
		else
		{
			BigCampsKilled.Add(Team, 1);
		}
		break;
	case ECampTypeForData::Boss:
		if (BossesKilled.Contains(Team))
		{
			BossesKilled[Team]++;
		}
		else
		{
			BossesKilled.Add(Team, 1);
		}
		break;
	}
}

void ADataGathererActor::SaveSnapshotsToJson()
{
	FString OutputString;
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	UE_LOG(LogTemp, Warning, TEXT("PlayerSnapshots.Num(): %d"), PlayerSnapshots.Num());
	for (const auto& PlayerSnapshot : PlayerSnapshots)
	{
		TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		bool bSuccess = FJsonObjectConverter::UStructToJsonObject(FPlayerSnapshot::StaticStruct(), &PlayerSnapshot,
		                                                          JsonObject, 0, 0);
		if (bSuccess)
		{
			JsonArray.Add(MakeShared<FJsonValueObject>(JsonObject));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to convert snapshot to JSON object."));
		}
	}

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonArray, Writer);


	if (JsonSavePath.IsEmpty())
	{
		JsonSavePath = FPaths::ProjectSavedDir() / TEXT("PlayerSnapshots" + FString::Printf(TEXT("_%s.json"), *FDateTime::Now().ToString()));
	}
	FString SavePath = JsonSavePath;
	isJsonLoaded = FFileHelper::SaveStringToFile(OutputString, *SavePath);
	if (isJsonLoaded)
	{
		UE_LOG(LogTemp, Log, TEXT("Snapshots JSON array saved to: %s"), *SavePath);
		// UE_LOG(LogTemp, Log, TEXT("Serialized JSON:\n%s"), *OutputString);
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple,
		//                                   FString::Printf(TEXT("Snapshots JSON array saved to: %s"), *SavePath));
		//make a copy of this json and add the date to the name
		FString NewSavePath = FPaths::ProjectSavedDir() / TEXT("PlayerSnapshots") / FString::Printf(TEXT("PlayerSnapshots_%s.json"), *FDateTime::Now().ToString());
		FFileHelper::SaveStringToFile(OutputString, *NewSavePath);
		JsonFileDone.Broadcast();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
		                                  TEXT("Failed to save snapshots JSON array."));
	}
}



TArray<float> ADataGathererActor::GetPlayerSoulsInreserve(FOwner Player, TArray<FDataGathererPlayerInfo> PlayerInfos)
{
	TArray<float> PlayerSoulsInReserve;
	for (const auto& PlayerInfo : PlayerInfos)
	{
		if (PlayerInfo.Owner.Player == Player.Player && PlayerInfo.Owner.Team == Player.Team)
		{
			PlayerSoulsInReserve.Add(PlayerInfo.SoulsInReserve);
		}
	}

	return PlayerSoulsInReserve;
}

TArray<float> ADataGathererActor::GetPlayerSoulsGain(FOwner Value, TArray<FDataGathererPlayerInfo> Array)
{
	return TArray<float>(); // TODO
}

TArray<float> ADataGathererActor::GetPlayerAmalgamsOnMapPerTeam(FOwner Value)
{
	ETeam Team = Value.Team;
	TArray<float> AmalgamsOnMapOfTeam;
	for (const auto& PlayerSnapshot : PlayerSnapshots)
	{
		AmalgamsOnMapOfTeam.Add(0.f);
		for (FAmalgamPresence AmalgamPositionsAndType : PlayerSnapshot.AmalgamPositionsAndType)
		{
			if (AmalgamPositionsAndType.Team == Team)
			{
				AmalgamsOnMapOfTeam[AmalgamsOnMapOfTeam.Num() - 1]++;
			}
		}
	}
	return AmalgamsOnMapOfTeam;
}

TArray<float> ADataGathererActor::GetPlayerBuildingCount(FOwner Value)
{
	ETeam Team = Value.Team;
	TArray<float> BuildingCountOfTeam;
	for (const auto& PlayerSnapshot : PlayerSnapshots)
	{
		BuildingCountOfTeam.Add(0.f);
		for (const auto& PlayerInfo : PlayerSnapshot.PlayerInfos)
		{
			if (PlayerInfo.Owner.Team == Team)
			{
				for (const auto& BuildingCount : PlayerInfo.BuildingsCountPerType)
				{
					BuildingCountOfTeam[BuildingCountOfTeam.Num() - 1] += BuildingCount.Value;
				}
			}
		}
	}
	return BuildingCountOfTeam;
}

TArray<float> ADataGathererActor::GetPlayerDominationPoints(FOwner Value, TArray<FDataGathererPlayerInfo> Array)
{
	TArray<float> DominationPointsOfTeam;
	ETeam Team = Value.Team;
	for (const auto& PlayerInfo : Array)
	{
		if (PlayerInfo.Owner.Team == Team)
		{
			DominationPointsOfTeam[DominationPointsOfTeam.Num() - 1] = PlayerInfo.DominationPoints;
		}
	}
	return DominationPointsOfTeam;
}

TArray<FString> ADataGathererActor::GetTimetimeMMSS()
{
	TArray<FString> TimeMMSS;
	for (const auto& PlayerSnapshot : PlayerSnapshots)
	{
		TimeMMSS.Add(PlayerSnapshot.TimeMMSS);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, PlayerSnapshot.TimeMMSS);
	}
	return TimeMMSS;
}

TArray<FPlayerSnapshot> ADataGathererActor::GetPlayerSnapshots()
{
	return PlayerSnapshots;
}


FPlayerChartData ADataGathererActor::GetPlayerInfos(FOwner Player, TArray<FDataGathererPlayerInfo> PlayerInfos,
                                                    EChartGetInfo ChartInfoToGet, TArray<FString> TimeMMSS)
{
	switch (ChartInfoToGet)
	{
	case EChartGetInfo::SoulsInReserve:
		return FPlayerChartData(GetPlayerSoulsInreserve(Player, PlayerInfos), TimeMMSS);
	case EChartGetInfo::SoulsGain:
		return FPlayerChartData(GetPlayerSoulsGain(Player, PlayerInfos), TimeMMSS);
	case EChartGetInfo::BuildingCount:
		return FPlayerChartData(GetPlayerBuildingCount(Player), TimeMMSS);
	case EChartGetInfo::AmalgamsOnMapPerTeam:
		return FPlayerChartData(GetPlayerAmalgamsOnMapPerTeam(Player), TimeMMSS);
	case EChartGetInfo::DominationPoints:
		return FPlayerChartData(GetPlayerDominationPoints(Player, PlayerInfos), TimeMMSS);
	default:
		return FPlayerChartData();
	}
}

void ADataGathererActor::TryLoad()
{
	
}
