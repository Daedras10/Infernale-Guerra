// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/Buildings/PandemoniumMainBuilding.h"

#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "GameMode/Infernale/GameModeInfernale.h"

float APandemoniumMainBuilding::GetVictoryPointsTimer() const
{
	return PandemoniumMBTimer;
}

float APandemoniumMainBuilding::GetVictoryPointsPerTime() const
{
	return PandemoniumMBPointsPerTime;
}

float APandemoniumMainBuilding::GetVictoryPointsTimer(const float Value) const
{
	return DelayBetweenRewards->GetFloatValue(Value);
}

float APandemoniumMainBuilding::GetVictoryPointsPerTime(const float Value) const
{
	return PointsAtDelaysRewards->GetFloatValue(Value);
}

void APandemoniumMainBuilding::OwnershipChanged()
{
	Super::OwnershipChanged();
}

void APandemoniumMainBuilding::GameModeInfernaleInitialized()
{
	Super::GameModeInfernaleInitialized();
	GameModeInfernale->GetVictoryManagerComponent()->AddPandemoniumMainBuilding(this);
}
