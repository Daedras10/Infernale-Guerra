// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerController/UIComponent.h"

#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "Component/PlayerController/FluxComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"

FVictoryInfo::FVictoryInfo(): VictoryPoints(0), VictoryRank(0), RankPos(0)
{
}

FVictoryInfoMap::FVictoryInfoMap()
{
}

FDeadTeam::FDeadTeam()
{
}

// Sets default values for this component's properties
UUIComponent::UUIComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


bool UUIComponent::IsTransmutationVisible() const
{
	return bIsTransmutationVisible;
}

void UUIComponent::SetTransmutationVisible(const bool bVisible)
{
	bIsTransmutationVisible = bVisible;
}

bool UUIComponent::ISConstructionMenuVisible() const
{
	return bConstructionMenuVisible;
}

void UUIComponent::SetConstructionMenuVisible(const bool bVisible)
{
	bConstructionMenuVisible = bVisible;
}

bool UUIComponent::ShouldCloseSomeMenus() const
{
	return bIsTransmutationVisible || bConstructionMenuVisible;
}

// Called when the game starts
void UUIComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority()) SubscribeToAuthorityEvents();
	if (PlayerControllerInfernale->IsLocalPlayerController()) SubscribeToLocalEvents();
}

void UUIComponent::SubscribeToAuthorityEvents()
{
	const auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("GameMode not found in UUIComponent::SubscribeToAuthorityEvents"));
		return;
	}
	const auto VictoryManagerComponent = GameMode->GetVictoryManagerComponent();
	
	VictoryManagerComponent->TimeRemainingDelegate.AddDynamic(this, &UUIComponent::OnTimeRemaining);
	VictoryManagerComponent->VictoryPointChanged.AddDynamic(this, &UUIComponent::OnVictoryPointChanged);
	VictoryManagerComponent->OnPlayerLost.AddDynamic(this, &UUIComponent::OnLocalPlayerLost);
	VictoryManagerComponent->OnGameEnded.AddDynamic(this, &UUIComponent::OnGameEnded);
	VictoryManagerComponent->OnLaunchSequence.AddDynamic(this, &UUIComponent::OnLaunchSequence);
	GameMode->AllPlayersReady.AddDynamic(this, &UUIComponent::OnAllPlayersReady);

	// Transmutation Events
	const auto TransmutationComponent = PlayerControllerInfernale->GetTransmutationComponent();
	TransmutationComponent->TransmutationQueueAltered.AddDynamic(this, &UUIComponent::OnTransmutationQueueAltered);
	TransmutationComponent->SimpleTransmutationQueueAltered.AddDynamic(this, &UUIComponent::OnSimpleTransmutationQueueAltered);
}

void UUIComponent::SubscribeToLocalEvents()
{
	// Flux Events
	const auto FluxComponent = PlayerControllerInfernale->GetFluxComponent();
	FluxComponent->FluxModeRemoving.AddDynamic(this, &UUIComponent::OnFluxModeRemoving);
	FluxComponent->FluxRemoving.AddDynamic(this, &UUIComponent::OnFluxRemoving);

	// Transmutation Events
	const auto TransmutationComponent = PlayerControllerInfernale->GetTransmutationComponent();
	TransmutationComponent->TransmutationMode.AddDynamic(this, &UUIComponent::OnTransmutationMode);
	TransmutationComponent->NodeOwnedAltered.AddDynamic(this, &UUIComponent::OnNodeOwnedAltered);
	TransmutationComponent->SimpleNodeOwnedAltered.AddDynamic(this, &UUIComponent::OnSimpleNodeOwnedAltered);
}

void UUIComponent::CheckTimeForNotification(const FTimeRemaining TimeRemaining)
{
	const float TimeRemainingValue = FMath::Floor(TimeRemaining.TimeRemaining);
	if (TimeRemainingValue <= 0) return;

	if (TimeRemainingValue == 10 * 60) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("10 minutes remaining"));
	if (TimeRemainingValue == 5 * 60) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("5 minutes remaining"));
	if (TimeRemainingValue == 1 * 60) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("1 minute remaining"));
	if (TimeRemainingValue == 30) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("30 seconds remaining"));
	if (TimeRemainingValue == 10) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("10 seconds remaining"));
	if (TimeRemainingValue == 9) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("9 seconds remaining"));
	if (TimeRemainingValue == 8) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("8 seconds remaining"));
	if (TimeRemainingValue == 7) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("7 seconds remaining"));
	if (TimeRemainingValue == 6) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("6 seconds remaining"));
	if (TimeRemainingValue == 5) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("5 seconds remaining"));
	if (TimeRemainingValue == 4) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("4 seconds remaining"));
	if (TimeRemainingValue == 3) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("3 seconds remaining"));
	if (TimeRemainingValue == 2) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("2 seconds remaining"));
	if (TimeRemainingValue == 1) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, TEXT("1 second remaining"));
}

FVictoryInfoMap UUIComponent::MakeVictoryInfoMap(const FVictoryPoints VictoryPoints, const FDeadTeam DeadPlayers)
{
	if (!PlayerControllerInfernale)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerControllerInfernale not found in UUIComponent::OnVictoryPointChangedClient"));
		return FVictoryInfoMap();
	}
	if (!PlayerControllerInfernale->PlayerState)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerState not found in UUIComponent::OnVictoryPointChangedClient"));
		return FVictoryInfoMap();
	}

	FVictoryInfoMap VictoryInfoMap;
	VictoryInfoMap.VictoryInfoMap = TMap<ETeam, FVictoryInfo>();
	VictoryInfoMap.VictoryInfoMap.Add(ETeam::Team1, FVictoryInfo());
	VictoryInfoMap.VictoryInfoMap.Add(ETeam::Team2, FVictoryInfo());
	VictoryInfoMap.VictoryInfoMap.Add(ETeam::Team3, FVictoryInfo());
	VictoryInfoMap.VictoryInfoMap.Add(ETeam::Team4, FVictoryInfo());

	VictoryInfoMap.VictoryInfoMap[ETeam::Team1].VictoryPoints = VictoryPoints.VictoryPointsTeam1;
	VictoryInfoMap.VictoryInfoMap[ETeam::Team2].VictoryPoints = VictoryPoints.VictoryPointsTeam2;
	VictoryInfoMap.VictoryInfoMap[ETeam::Team3].VictoryPoints = VictoryPoints.VictoryPointsTeam3;
	VictoryInfoMap.VictoryInfoMap[ETeam::Team4].VictoryPoints = VictoryPoints.VictoryPointsTeam4;

	const auto GameInstance = Cast<UGameInstanceInfernale>(GetWorld()->GetGameInstance());
	const auto OppenentInfos = GameInstance->OpponentPlayerInfo;


	int ValidElems = 0;
	for (const auto& TeamWithScore : VictoryInfoMap.VictoryInfoMap)
	{
		if (TeamWithScore.Value.VictoryPoints == -1) continue;
		ValidElems++;
		int ScoreIndex = 0;
		
		for (const auto& TeamVPoint : VictoryInfoMap.VictoryInfoMap)
		{
			if (TeamVPoint.Value.VictoryPoints == -1) continue;
			if (TeamWithScore.Key == TeamVPoint.Key) continue;
			if (TeamVPoint.Value.VictoryPoints > TeamWithScore.Value.VictoryPoints) ScoreIndex++;
		}
		
		VictoryInfoMap.VictoryInfoMap[TeamWithScore.Key].VictoryRank = ScoreIndex;
		VictoryInfoMap.VictoryInfoMap[TeamWithScore.Key].PlayerAlive = !(DeadPlayers.DeadTeams.Contains(TeamWithScore.Key));

		for (auto OppenentInfo : OppenentInfos)
		{
			// int PlayerID = (int)Elem.Key-1;
			// if (OppenentInfo.PlayerID != PlayerID) continue;
			const auto OwnerLoc = OppenentInfo.PlayerOwnerInfo;
			if (OwnerLoc.Team != TeamWithScore.Key) continue;
			VictoryInfoMap.VictoryInfoMap[TeamWithScore.Key].PlayerName = OppenentInfo.PlayerName;
			break;
		}
	}

	int ScoreRanked = 0;
	int LastHighest = MAX_int32;
	TArray<ETeam> Teams = TArray<ETeam>();

	int Safety = 0;
	while (ScoreRanked < ValidElems && Safety < 100)
	{
		int CurrentHighest = -1;
		Safety++;
		Teams.Empty();
		
		for (const auto& TeamVPoint : VictoryInfoMap.VictoryInfoMap)
        {
            if (TeamVPoint.Value.VictoryPoints == -1) continue;
			const auto CurrentTeamPoints = TeamVPoint.Value.VictoryPoints;
            if (CurrentHighest < CurrentTeamPoints && CurrentTeamPoints < LastHighest)
            {
            	Teams.Empty();
	            CurrentHighest = TeamVPoint.Value.VictoryPoints;
            }
			if (CurrentHighest == TeamVPoint.Value.VictoryPoints) Teams.Add(TeamVPoint.Key);
        }
		LastHighest = CurrentHighest;

		for (int i = 0; i < Teams.Num(); i++)
        {
            VictoryInfoMap.VictoryInfoMap[Teams[i]].RankPos = ScoreRanked;
            ScoreRanked++;
        }
	}
	
	//bool IsValid = FVictoryInfoMapValidity(VictoryInfoMap);
	// if (!IsValid)
	// {
	// 	/* Try to repair the Map */
	// }

	/* Debug team points */
	// UE_LOG(LogTemp, Warning, TEXT("----------------------------------------------------------------------------"));
	// //UE_LOG(LogTemp, Warning, TEXT("VictoryInfoMap Valid: %s"), IsValid ? TEXT("true") : TEXT("false"));
	// for (const auto& TeamWithScore : VictoryInfoMap.VictoryInfoMap)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Team: %s, Points: %d, Rank: %d, RankPos: %d, Alive: %s, Name: %s"),
	// 		*UEnum::GetValueAsString(TeamWithScore.Key),
	// 		TeamWithScore.Value.VictoryPoints,
	// 		TeamWithScore.Value.VictoryRank,
	// 		TeamWithScore.Value.RankPos,
	// 		TeamWithScore.Value.PlayerAlive ? TEXT("true") : TEXT("false"),
	// 		*TeamWithScore.Value.PlayerName);
	// }
	// UE_LOG(LogTemp, Warning, TEXT("----------------------------------------------------------------------------"));

	
	
	return VictoryInfoMap;
}

bool UUIComponent::FVictoryInfoMapValidity(const FVictoryInfoMap& VictoryInfoMap) const
{
	TArray<int> RanksInts = TArray<int>();
	RanksInts.Empty();
	RanksInts.Add(0);
	RanksInts.Add(0);
	RanksInts.Add(0);
	RanksInts.Add(0);
	RanksInts.Add(0);
	for (const auto& TeamVPoint : VictoryInfoMap.VictoryInfoMap)
	{
		if (TeamVPoint.Value.VictoryPoints == -1) continue;
		RanksInts[TeamVPoint.Value.RankPos]++;
	}

	bool IsValid = true;
	bool ZeroFound = false;
	for (int i = 0; i < RanksInts.Num(); i++)
	{
		if (RanksInts[i] == 0)
		{
			ZeroFound = true;
			continue;
		}
		if (ZeroFound)
		{
			IsValid = false;
			break;
		}
		RanksInts[i]++;
	}
	
	for (int i = 0; i < RanksInts.Num(); i++)
	{
		if (RanksInts[i] > 1)
		{
			IsValid = false;
			break;
		}
	}
	return IsValid;
}

void UUIComponent::OnTimeRemaining(const FTimeRemaining TimeRemaining)
{
	UpdateTimeRemainingClient(TimeRemaining);
}

void UUIComponent::OnVictoryPointChanged(const FVictoryPoints VictoryPoints,
	const EVictoryPointReason VictoryPointReason, const ETeam TeamPointChanged, int Score)
{
	OnVictoryPointChangedClient(VictoryPoints, VictoryPointReason, TeamPointChanged, Score);
}

void UUIComponent::OnFluxModeRemoving(bool Start, EFluxModeState FluxModeState, float Duration)
{
	OnFluxModeRemovingBP(Start, FluxModeState, Duration);
}

void UUIComponent::OnFluxRemoving(EFluxModeState FluxModeState, float CurrentTimer, float MaxTimer)
{
	OnFluxRemovingBP(FluxModeState, CurrentTimer, MaxTimer);
}

void UUIComponent::OnTransmutationMode(const bool bTransmutationMode)
{
	OnTransmutationModeBP(bTransmutationMode);
}

void UUIComponent::CreateTransmutationNodesMulticast_Implementation(
	const TArray<FTransmutationNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings)
{
	if (!PlayerControllerInfernale)
	{
		PlayerControllerInfernale = Cast<APlayerControllerInfernale>(GetOwner());
	}
	
	/* Check if this is a local player */
	if (!PlayerControllerInfernale->IsLocalPlayerController()) return;

	CreateTransmutationNodesBP(TransmutationNodesOwned, TransmutationSettings);
}

void UUIComponent::CreateTransmutationSimpleNodesMulticast_Implementation(
	const TArray<FTransmutationSimpleNodeOwned>& TransmutationNodesOwned,
	FTransmutationSettings TransmutationSettings)
{
	if (!PlayerControllerInfernale)
	{
		PlayerControllerInfernale = Cast<APlayerControllerInfernale>(GetOwner());
	}
	
	/* Check if this is a local player */
	if (!PlayerControllerInfernale->IsLocalPlayerController()) return;

	CreateTransmutationSimpleNodesBP(TransmutationNodesOwned, TransmutationSettings);
}

void UUIComponent::CreateLoadingUIBP_Implementation()
{
}

void UUIComponent::OnVictoryPointChangedBP_Implementation(FVictoryInfoMap VictoryInfo, const EVictoryPointReason VictoryPointReason, const ETeam TeamPointChanged, int Score)
{
	// Override this function in blueprint
}

void UUIComponent::OnVictoryPointChangedClient_Implementation(const FVictoryPoints VictoryPoints, const EVictoryPointReason VictoryPointReason, const ETeam TeamPointChanged, int Score)
{
	FVictoryInfoMap VictoryInfoMap = MakeVictoryInfoMap(VictoryPoints, FDeadTeam());
	if (VictoryInfoMap.VictoryInfoMap.Num() == 0) return;
	OnVictoryPointChangedBP(VictoryInfoMap, VictoryPointReason, TeamPointChanged, Score);
}

void UUIComponent::UpdateTimeRemainingClient_Implementation(const FTimeRemaining TimeRemaining)
{
	CheckTimeForNotification(TimeRemaining);
	UpdateTimeRemainingBP(TimeRemaining);
}


// Called every frame
void UUIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UUIComponent::CreateTransmutationNodes(const TArray<FTransmutationNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings)
{
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel;
	
	TimerDel.BindUFunction(this, FName("CreateTransmutationNodesMulticast"), TransmutationNodesOwned, TransmutationSettings);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 1.f, false);
}

void UUIComponent::CreateTransmutationSimpleNodes(const TArray<FTransmutationSimpleNodeOwned>& TransmutationNodesOwned, FTransmutationSettings TransmutationSettings)
{
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel;
	
	TimerDel.BindUFunction(this, FName("CreateTransmutationSimpleNodesMulticast"), TransmutationNodesOwned, TransmutationSettings);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 1.f, false);
}

void UUIComponent::FluxEntityHovered(const EFluxHoverInfoType InfoType) const
{
	FluxEntityHoveredBP(InfoType);
}

void UUIComponent::SoulBeaconSelected(ASoulBeacon* Beacon, bool Selected)
{
	SoulBeaconSelectedClient(Beacon, Selected);
}

void UUIComponent::OnNodeOwnedAltered(const FString& NodeID, const FTransmutationQueue TransmutationQueue)
{
	OnNodeOwnedAlteredBP(NodeID, TransmutationQueue);
}

void UUIComponent::OnSimpleNodeOwnedAltered(const FString& NodeID, const FSimpleTransmutationQueue TransmutationQueue)
{
	OnSimpleNodeOwnedAlteredBP(NodeID, TransmutationQueue);
}

void UUIComponent::OnTransmutationQueueAltered(const FTransmutationQueue TransmutationQueue)
{
	OnTransmutationQueueAlteredClient(TransmutationQueue);
}

void UUIComponent::OnSimpleTransmutationQueueAltered(const FSimpleTransmutationQueue TransmutationQueue)
{
	OnSimpleTransmutationQueueAlteredClient(TransmutationQueue);
}

void UUIComponent::OnAllPlayersReady()
{
	const auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	const auto GameInstance = Cast<UGameInstanceInfernale>(GameMode->GetGameInstance());
	const int Players = GameInstance->GetPlayers();
	OnAllPlayersSpawnedClient(Players);
}

void UUIComponent::OnLocalPlayerLost(const FVictoryPoints VictoryPoints, TArray<ETeam> DeadPlayers, ETeam Team)
{
	const auto Playerstate = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
	if (Team != Playerstate->GetOwnerInfo().Team) return;
	FDeadTeam DeadTeam;
	DeadTeam.DeadTeams = TArray<ETeam>(DeadPlayers);
	OnLocalPlayerLostClient(VictoryPoints, DeadTeam);
}

void UUIComponent::OnGameEnded(const FVictoryPoints VictoryPoints, TArray<ETeam> DeadPlayers, ETeam Team)
{
	FDeadTeam DeadTeam;
	DeadTeam.DeadTeams = TArray<ETeam>(DeadPlayers);
	OnGameEndedClient(VictoryPoints, DeadTeam);
}

void UUIComponent::OnLaunchSequence(const FVictoryPoints VictoryPoints, TArray<ETeam> Teams, ETeam Team)
{
	FDeadTeam DeadTeam;
	DeadTeam.DeadTeams = TArray<ETeam>(Teams);
	OnLaunchSequenceClient(VictoryPoints, DeadTeam, Team);
}

void UUIComponent::OnLaunchSequenceClient_Implementation(const FVictoryPoints& VictoryPoints, const FDeadTeam& DeadTeam, ETeam Team)
{
	OnLaunchSequenceBP(VictoryPoints, Team);
}

void UUIComponent::OnLaunchSequenceBP_Implementation(const FVictoryPoints& VictoryPoints, ETeam Team)
{
}

void UUIComponent::OnTransmutationQueueAlteredClient_Implementation(const FTransmutationQueue TransmutationQueue)
{
	OnTransmutationQueueAlteredBP(TransmutationQueue);
}

void UUIComponent::OnSimpleTransmutationQueueAlteredClient_Implementation(
	const FSimpleTransmutationQueue TransmutationQueue)
{
	OnSimpleTransmutationQueueAlteredBP(TransmutationQueue);
}

void UUIComponent::OnGameEndedClient_Implementation(const FVictoryPoints VictoryPoints, const FDeadTeam DeadPlayers)
{
	FVictoryInfoMap VictoryInfoMap = MakeVictoryInfoMap(VictoryPoints, DeadPlayers);
	if (VictoryInfoMap.VictoryInfoMap.Num() == 0) return;
	const auto GameInstance = Cast<UGameInstanceInfernale>(GetWorld()->GetGameInstance());
	GameInstance->SetVictoryInfo(VictoryInfoMap);
	OnGameEndedBP(VictoryInfoMap);
}

void UUIComponent::OnGameEndedBP_Implementation(FVictoryInfoMap VictoryInfo)
{
}

void UUIComponent::OnLocalPlayerLostBP_Implementation(FVictoryInfoMap VictoryInfo)
{
}

void UUIComponent::OnLocalPlayerLostClient_Implementation(const FVictoryPoints VictoryPoints, const FDeadTeam DeadPlayers)
{
	FVictoryInfoMap VictoryInfoMap = MakeVictoryInfoMap(VictoryPoints, DeadPlayers);
	if (VictoryInfoMap.VictoryInfoMap.Num() == 0) return;
	OnLocalPlayerLostBP(VictoryInfoMap);
}

void UUIComponent::OnAllPlayersSpawnedClient_Implementation(int Players)
{
	OnAllPlayersSpawnedBP(Players);
}

void UUIComponent::SoulBeaconSelectedClient_Implementation(ASoulBeacon* Beacon, bool Selected)
{
	SoulBeaconSelectedBP(Beacon, Selected);
}

void UUIComponent::SoulBeaconSelectedBP_Implementation(ASoulBeacon* Beacon, bool Selected)
{
}
