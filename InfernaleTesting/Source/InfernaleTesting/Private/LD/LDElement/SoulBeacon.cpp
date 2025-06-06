// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/LDElement/SoulBeacon.h"

#include "Component/ActorComponents/AttackComponent.h"
#include "Component/ActorComponents/DamageableComponent.h"
#include "Component/ActorComponents/EffectAfterDelayComponent.h"
#include "Component/ActorComponents/UnitActorGridComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "DataAsset/SoulBeaconRewardAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Manager/UnitActorManager.h"
#include <Mass/Collision/SpatialHashGrid.h>
#include "Component/PlayerController/UIComponent.h"


FDetectionRange::FDetectionRange(): DetectionPivot(nullptr), Radius(0)
{
}

ASoulBeacon::ASoulBeacon()
{
	EffectAfterDelayComponent = CreateDefaultSubobject<UEffectAfterDelayComponent>(TEXT("EffectAfterDelayComponent"));
	DamageableComponent = CreateDefaultSubobject<UDamageableComponent>(TEXT("DamageableComponent"));
	
	PrimaryActorTick.bCanEverTick = true;
	LDElementType = ELDElementType::LDElementSoulBeaconType;
}

void ASoulBeacon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority()) return;

	Capture(DeltaTime);
	if (LastCaptureAmountSoulBeaconSent != CaptureAmountSoulBeacon)
	{
		CurrentReplicationTime += DeltaTime;
		if (CurrentReplicationTime >= ReplicationTime)
		{
			LastCaptureAmountSoulBeaconSent = CaptureAmountSoulBeacon;
			CurrentReplicationTime = 0.f;
			SetCaptureAmountSoulBeaconMulticast(CaptureAmountSoulBeacon, MainContesterSoulBeacon);
		}
	}
	//Activate(DeltaTime);
}

TWeakObjectPtr<UDamageableComponent> ASoulBeacon::GetDamageableComponent() const
{
	//return DamageableComponent;
	return nullptr; 
}

FOwner ASoulBeacon::GetOwner()
{
	return OwnerWithTeam;
}

void ASoulBeacon::SetOwner(FOwner NewOwner)
{
	OwnerWithTeam = NewOwner;
}

void ASoulBeacon::ChangeOwner(FOwner NewOwner)
{
	SetOwner(NewOwner);
}

void ASoulBeacon::TryGetGameModeInfernale()
{
	if (GameModeInfernaleWasInitialized) return;
	const auto World = GetWorld();
	GameModeInfernale = Cast<AGameModeInfernale>(World->GetAuthGameMode());

	if (!GameModeInfernale)
	{
		FTimerHandle TimerHandle_GameModeInfernale;
		World->GetTimerManager().SetTimer(TimerHandle_GameModeInfernale, this, &ASoulBeacon::TryGetGameModeInfernale, 0.1f, false);
		return;
	}

	GameModeInfernale->PreLaunchGame.AddDynamic(this, &ASoulBeacon::OnPreLaunchGame);
	GameModeInfernaleWasInitialized = true;

}

void ASoulBeacon::OnPreLaunchGame()
{
	ASpatialHashGrid::AddSoulBeaconToGrid(this);
	FOwner DefaultOwner;
	DefaultOwner.Player = EPlayerOwning::Nature;
	DefaultOwner.Team = ETeam::NatureTeam;
	Captured(DefaultOwner);
}

float ASoulBeacon::DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner)
{
	if(!HasAuthority()) return 0.0f;
	//return DamageableComponent->DamageHealthOwner(DamageAmount, bDamagePercent, DamageOwner);
	return 0.f;
}

float ASoulBeacon::GetTargetableRange()
{
	return EffectRange;
}

EEntityType ASoulBeacon::GetEntityType() const
{
	return Super::GetEntityType();
}

TArray<FDetectionRange> ASoulBeacon::GetCaptureRanges()
{
	return CaptureRanges;
}

TArray<FDetectionRange> ASoulBeacon::GetEffectRanges()
{
	return EffectRanges;
}

void ASoulBeacon::Reward(ESoulBeaconRewardType RewardType)
{
	if (!IsValidRewarder()) return;

	const auto PlayerStateInfernale = GameModeInfernale->GetPlayerState(GetOwner().Player);
	int Reward = 0;
	if (!PlayerStateInfernale.IsValid())
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerStateInfernale not found in ASoulBeacon::Reward"));
		return;
	}

	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("SoulBeacon rewarded player %d"), GetOwner().Team));

	if (!RewardDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Dataasset not found in ASoulBeacon::Reward"));
		return;
	}
	
	switch (RewardType)
	{
	case RewardAmalgam:
		Reward = RewardDataAsset->AmalgamReward;
		break;
	case RewardNeutral:
		Reward = RewardDataAsset->NeutralReward;
		break;
	case RewardBoss:
		Reward = RewardDataAsset->BossReward;
		break;
	case RewardBuilding:
		Reward = RewardDataAsset->BuildingReward;
		break;
	case RewardMainBuilding:
		Reward = RewardDataAsset->MainBuildingReward;
		break;
	}
	
	PlayerStateInfernale->GetEconomyComponent()->AddSouls(this, ESoulsGainCostReason::SoulBeaconReward, Reward);
}

void ASoulBeacon::RewardMultiple(ESoulBeaconRewardType RewardType, int RewardCount)
{
	if (!IsValidRewarder()) return;
	
	if (RewardCount == 0) return;

	const auto PlayerStateInfernale = GameModeInfernale->GetPlayerState(GetOwner().Player);
	int Reward = 0;
	if (!PlayerStateInfernale.IsValid())
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerStateInfernale not found in ASoulBeacon::Reward"));
		return;
	}

	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("SoulBeacon rewarded player %d"), GetOwner().Team));
	switch (RewardType)
	{
	case RewardAmalgam:
		Reward = RewardDataAsset->AmalgamReward;
		break;
	case RewardNeutral:
		Reward = RewardDataAsset->NeutralReward;
		break;
	case RewardBoss:
		Reward = RewardDataAsset->BossReward;
		break;
	case RewardBuilding:
		Reward = RewardDataAsset->BuildingReward;
		break;
	case RewardMainBuilding:
		Reward = RewardDataAsset->MainBuildingReward;
		break;
	}

	PlayerStateInfernale->GetEconomyComponent()->AddSouls(this, ESoulsGainCostReason::SoulBeaconReward, Reward * RewardCount);
	
	RewardMulticast(RewardType);
	//RewardBP(RewardType);
}

void ASoulBeacon::RewardMulticast_Implementation(ESoulBeaconRewardType RewardType)
{
	RewardBP(RewardType);
}

void ASoulBeacon::SetCaptureAmountSoulBeaconMulticast_Implementation(float Amount, FOwner MainContesterInfo)
{
	if (HasAuthority()) return;
	CaptureAmountSoulBeacon = Amount;
	MainContesterSoulBeacon = MainContesterInfo;
}

void ASoulBeacon::BeginPlay()
{
	Super::BeginPlay();
	SyncDataAsset();

	if (!HasAuthority()) return;

	OnContesterAddOrRemove.AddDynamic(this, &ASoulBeacon::UpdateState);
	OnCaptured.AddDynamic(this, &ASoulBeacon::Captured);

	/*DamageableComponent->HealthDepeltedOwner.AddDynamic(this, &ASoulBeacon::OnCaptured);
	DamageableComponent->Damaged.AddDynamic(this, &ASoulBeacon::OnDamaged);*/

	DamageableComponent->SetMaxHealth(MaxHealth, true, true, false, MaxHealth);

	TryGetGameModeInfernale();

	//ASpatialHashGrid::AddSoulBeaconToGrid(this);
}

void ASoulBeacon::RegisterLDElement()
{
	// Nothing for now
}

void ASoulBeacon::InteractStartMain(APlayerControllerInfernale* Interactor)
{
	Interactor->GetUIComponent()->SoulBeaconSelected(this, true);
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("Interaction w/beacon")));
}

void ASoulBeacon::InteractEndMain(APlayerControllerInfernale* Interactor)
{
	Interactor->GetUIComponent()->SoulBeaconSelected(this, false);
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("Interaction w/beacon ended")));
}

void ASoulBeacon::ActivateInstant()
{
	if (!CanActivate()) return;

	bIsActive = true;
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("Soul beacon Effect : ACTIVATED ")));
	
	// @TODO : ACTIVATION
}

bool ASoulBeacon::CanActivate()
{
	return !bIsActive;
}

void ASoulBeacon::SyncDataAsset()
{
	// See if actually useful
}

void ASoulBeacon::OnDamaged(TWeakObjectPtr<AActor> _, float NewHealth, float DamageAmount)
{
	// See if actually used
}

void ASoulBeacon::UpdateOwnerVisuals(TWeakObjectPtr<ASoulBeacon> Actor, FOwner OldOwner, FOwner NewOwner)
{
	Execute_NewOwnerVisuals(this, Actor.Get(), OldOwner, NewOwner);
}


void ASoulBeacon::OnOwnerChangedMulticast_Implementation(FOwner NewOwner)
{
	UpdateOwnerVisuals(this, OwnerWithTeam, NewOwner);
	OnCapturedVFXTrigger(GetActorLocation());
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Beacon captured by team %d"), NewOwner.Team));
	
	if (!HasAuthority()) SetOwner(NewOwner);
}

//void ASoulBeacon::VisualDamagedBP(float NewHealth, float DamageAmount, float Percent)
//{
//	// Use to update
//}

void ASoulBeacon::Capture(float DeltaTime)
{
	if (OwnerWithTeam.Team != ETeam::NatureTeam)
		GEngine->AddOnScreenDebugMessage(1, 2.5f, FColor::Cyan, FString::Printf(TEXT("%d contesting players"), Contesters.Num()));

	switch (CaptureState)
	{
	case Capturing:
		if (Contesters[0] != MainContesterSoulBeacon)
		{
			if (CaptureAmountSoulBeacon < ContesterChangeThreshold)
				MainContesterSoulBeacon = Contesters[0];
			else
				CaptureAmountSoulBeacon -= CapturePerSecond * DeltaTime;
		}
		else
		{
			CaptureAmountSoulBeacon += CapturePerSecond * DeltaTime;
			if (CaptureAmountSoulBeacon > 100.f)
			{
				if (GetOwner().Team != ETeam::NatureTeam)
				{
					FOwner NatureOwner;
					NatureOwner.Team = ETeam::NatureTeam;
					NatureOwner.Player = EPlayerOwning::Nature;
					OnCaptured.Broadcast(NatureOwner);
				}
				else
					OnCaptured.Broadcast(MainContesterSoulBeacon);

			}
		}

		break;
	case Uncontested:
		CaptureAmountSoulBeacon -= CaptureDecreasePerSecond * DeltaTime;
		if (CaptureAmountSoulBeacon < 0.f) CaptureAmountSoulBeacon = 0.f;
		break;
	default:
		return;
	}
}

void ASoulBeacon::Captured(FOwner Capturer)
{
	if (!HasAuthority()) return;

	SetOwner(Capturer);
	CaptureAmountSoulBeacon = 0.f;
	UpdateState();
	OnOwnerChangedMulticast(Capturer);
}

void ASoulBeacon::DebugCaptureRange()
{
	for (auto& Range : GetCaptureRanges())
	{
		DrawDebugSphere(GetWorld(), Range.GetPosition(), Range.Radius, 10, FColor::Red, false, 10.f);
	}
}

void ASoulBeacon::DebugEffectRange()
{
	for (auto& Range : GetEffectRanges())
	{
		DrawDebugSphere(GetWorld(), Range.GetPosition(), Range.Radius, 10, FColor::Purple, false, 10.f);
	}
}

void ASoulBeacon::AddContester(FOwner Contester)
{
	if (!Contesters.Contains(Contester))
	{
		Contesters.Add(Contester);
		OnContesterAddOrRemove.Broadcast();
	}
}

void ASoulBeacon::RemoveContester(FOwner Contester)
{
	if (Contesters.Contains(Contester))
	{
		Contesters.Remove(Contester);
		OnContesterAddOrRemove.Broadcast();
	}

}

void ASoulBeacon::AddCells(TArray<HashGridCell*> AddedCells)
{
	for (const auto& Cell : AddedCells)
		AddCell(Cell);
}

void ASoulBeacon::AddCell(HashGridCell* Cell)
{
	CaptureCells.Add(Cell);
}

void ASoulBeacon::CheckCells()
{
	if (CaptureCells.Num() == 0) return;
	TArray<FOwner> AllOwnersInRange;
	for (auto& Cell : CaptureCells)
	{
		for (auto& PresentOwner : Cell->GetPresentOwners())
			if(!AllOwnersInRange.Contains(PresentOwner))
				AllOwnersInRange.Add(PresentOwner);
	}

	for (auto& OwnerInRange : AllOwnersInRange)
	{
		if (!Contesters.Contains(OwnerInRange)) AddContester(OwnerInRange);
	}

	TArray<FOwner> ContesterCopy = Contesters; // Should prevent In-loop removing from causing issues ?
	for (auto& Contester : ContesterCopy)
	{
		if (!AllOwnersInRange.Contains(Contester)) RemoveContester(Contester);
	}
}

bool ASoulBeacon::IsCellInRange(HashGridCell* Cell)
{
	return CaptureCells.Contains(Cell);
}

bool ASoulBeacon::IsValidRewarder()
{
	return GetOwner().Team != ETeam::NatureTeam;
}

void ASoulBeacon::UpdateState()
{
	int NumberOfConsters = Contesters.Num();

	ESoulBeaconCaptureState NewCaptureState = CaptureState;

	switch (NumberOfConsters)
	{
	case 0:
		NewCaptureState = ESoulBeaconCaptureState::Uncontested;
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5, FColor::Orange, TEXT("SoulBeacon is now Uncontested"));
		break;

	case 1:
		NewCaptureState = Contesters[0].Team == GetOwner().Team
			? ESoulBeaconCaptureState::Uncontested
			: ESoulBeaconCaptureState::Capturing;
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5, FColor::Orange, TEXT("SoulBeacon has a single contester"));		
		break;

	default:
		NewCaptureState = ESoulBeaconCaptureState::Stalemate;
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5, FColor::Orange, TEXT("SoulBeacon capture in Stalemate"));
		break;
	}
	
	if (NewCaptureState == CaptureState) return;

	CaptureState = NewCaptureState;
	switch (CaptureState)
	{
	case ESoulBeaconCaptureState::Uncontested:
		OnUncontestedBP();
		break;

	case ESoulBeaconCaptureState::Capturing:
		OnCapturingBP();
		break;

	case ESoulBeaconCaptureState::Stalemate:
		OnStalemateBP();
		break;
	}

}