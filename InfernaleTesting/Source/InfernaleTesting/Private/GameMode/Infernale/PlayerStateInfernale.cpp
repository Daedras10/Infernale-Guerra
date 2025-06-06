// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/Infernale/PlayerStateInfernale.h"

#include "Component/PlayerState/EconomyComponent.h"
#include "DataAsset/BuildingListDataAsset.h"
#include "GameMode/GameInstanceInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"

APlayerStateInfernale::APlayerStateInfernale()
{
	EconomyComponent = CreateDefaultSubobject<UEconomyComponent>(TEXT("EconomyComponent"));
}

UEconomyComponent* APlayerStateInfernale::GetEconomyComponent()
{
	return EconomyComponent;
}

TMap<EPlayerOwning, FString> APlayerStateInfernale::GetPlayerOwningNameMap() const
{
	return PlayerOwningNameMap;
}

void APlayerStateInfernale::BeginPlay()
{
	Super::BeginPlay();

	// Chek if we are the owner of this playerstate
	// if (GetLocalRole() == ROLE_AutonomousProxy || GetLocalRole() == ROLE_Authority)
	// {
	// 	// We are the owner of this playerstate
	// 	OwnerInfo.Player = EPlayerOwning::Player1;
	// 	OwnerInfo.Team = ETeam::Team1;
	// 	SetOwnerInfo(OwnerInfo);
	//
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PS: Owned locally"));
	// }
	
	// if (HasAuthority())
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PS: HasAuthority"));
	// 	//TryWarnGameModeInfernale();
	// }
}

void APlayerStateInfernale::TryWarnGameModeInfernale()
{
	const auto World = GetWorld();
	if (const auto GameModeInfernale = Cast<AGameModeInfernale>(World->GetAuthGameMode()))
	{
		GameModeInfernale->AddPlayerState(GetOwnerInfo(), this);
		return;
	}
	
	FTimerHandle TimerHandle_GameModeInfernale;
	World->GetTimerManager().SetTimer(TimerHandle_GameModeInfernale, this, &APlayerStateInfernale::TryWarnGameModeInfernale, 0.1f, false);
}

void APlayerStateInfernale::BroadCastPlayerStateOwnerChanged_Implementation(FOwner NewOwner)
{
	PlayerStateOwnerChanged.Broadcast(this, NewOwner);
}

void APlayerStateInfernale::SetOwnerInfoMulticast_Implementation(FOwner NewOwner)
{
	OwnerInfo = NewOwner;
	PlayerStateOwnerChanged.Broadcast(this, OwnerInfo);
}

void APlayerStateInfernale::SetPlayerNamesMulticast_Implementation(const TArray<EPlayerOwning>& PlayerOwning, const TArray<FString>& PlayerNames)
{
	PlayerOwningNameMap.Empty();
	for (int i = 0; i < PlayerOwning.Num(); i++)
	{
		PlayerOwningNameMap.Add(PlayerOwning[i], PlayerNames[i]);
	}
}

TArray<FCategoryStruct> APlayerStateInfernale::GetBuildingListAvailable()
{
	auto BuildingList = TArray<FCategoryStruct>(GetBuildingListDataAsset->BuildingsCategories);
	// Should be implemented to filter out the buildings that are not available to the player
	return BuildingList;
}

void APlayerStateInfernale::Init(const FOwner NewOwner)
{
	SetOwnerInfo(NewOwner);
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerState: Init"));
	BroadCastPlayerStateOwnerChanged(NewOwner);
	if (!bDebug) return;
	switch (NewOwner.Team) {
	case ETeam::NatureTeam:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PS: NatureTeam"));
		break;
	case ETeam::Team1:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PS: Team1"));
		break;
	case ETeam::Team2:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PS: Team2"));
		break;
	case ETeam::Team3:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PS: Team3"));
		break;
	case ETeam::Team4:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PS: Team4"));
		break;
	}
}

FOwner APlayerStateInfernale::GetOwnerInfo()
{
	if (OwnerInfo.Player == EPlayerOwning::Nature || OwnerInfo.Team == ETeam::NatureTeam)
	{
		AskResynchServer();
		//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("PS: NatureTeam or NaturePlayer, trying to resynch"));
		return OwnerInfo;
	}
	
	return OwnerInfo;
}

FOwner APlayerStateInfernale::GetOwnerInfoBP()
{
	return GetOwnerInfo();
}

void APlayerStateInfernale::SetPlayerNamesServer_Implementation(const TArray<EPlayerOwning>& PlayerOwning,
	const TArray<FString>& PlayerNames)
{
	SetPlayerNamesMulticast(PlayerOwning, PlayerNames);
}

void APlayerStateInfernale::AskResynchServer_Implementation()
{
	const auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return;

	GameMode->ResynchPS(this);
}

void APlayerStateInfernale::SetOwnerInfo_Implementation(FOwner NewOwner)
{
	SetOwnerInfoMulticast(NewOwner);
}


