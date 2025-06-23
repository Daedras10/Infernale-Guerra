// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(Blueprintable)
enum class EChartGetInfo : uint8
{
	SoulsInReserve UMETA(DisplayName = "Souls In Reserve"),
	SoulsGain UMETA(DisplayName = "Souls Gain"),
	BuildingCount UMETA(DisplayName = "Building Count"),
	AmalgamsOnMapPerTeam UMETA(DisplayName = "Number Of Units On Map Per Team"),
	DominationPoints UMETA(DisplayName = "Domination Points"),
	ArmyVolumePerTeam UMETA(DisplayName = "Army Volume Per Team"),
};

UENUM(Blueprintable)
enum class EPlayerOwning : uint8
{
	Nature,
	Player1,
	Player2,
	Player3,
	Player4
};

UENUM(Blueprintable)
enum class ETeam : uint8
{
	NatureTeam,
	Team1,
	Team2,
	Team3,
	Team4
};

UENUM(Blueprintable)
enum EAttackType
{
	Single,
	Multiple
};

UENUM()
enum EUnitStateAsk
{
	UnitStateAskNone,
	UnitStateAskRange,
	UnitStateAskAttack,
};

UENUM(Blueprintable)
enum class EStat : uint8
{
	StatNone,
	StatAttack,
	StatMaxHealth,
	StatSpeed,
};

UENUM(Blueprintable)
enum class EUnitTargetType : uint8
{
	UTargetNone,
	UTargetBuilding,
	UTargetUnit,
	UTargetNeutralCamp,
};

UENUM()
enum EAmalgamState : uint8
{
	Inactive,
	FollowPath,
	Aggroed,
	Fighting,
	Killed
};

UENUM()
enum EAmalgamAggro : uint8
{
	NoAggro,
	Amalgam,
	Building,
	LDElement
};

UENUM(Blueprintable)
enum EAmalgamDeathReason : uint8
{
	NoReason,
	EndOfPath,
	Eliminated, // Killed already in use
	Sacrificed, // used to charge boss 
	Fell, // if entity enters kill zone
	Error,
};

UENUM()
enum EAmalgamAggroPriority : uint8
{
	Standard, // Focuses the closest found element
	AmalgamFirst, // Focuses the closest amalgam found if any, standard else
	BuildingFirst, // Focuses the closest building found if any, standard else
	LDFirst, // qui utiliserai �a en sah
};

UENUM(Blueprintable)
enum class EEntityType : uint8
{
	EntityTypeNone,
	EntityTypeBehemot,
	EntityTypeGobborit,
	EntityTypeNerras,
	EntityTypeNeutralCamp,
	EntityTypeBoss,
	EntityTypeCity,
	EntityTypeBuilding,
};

UENUM(Blueprintable)
enum class ENodeEffect : uint8
{
	NodeEffectNone,
	NodeEffectDamageToBuilding,
	NodeEffectDamageToMonster,
	NodeEffectDamageToUnit,
	NodeEffectHealthUnit,
	NodeEffectHealthBuilding,
	NodeEffectFluxRange,
	NodeEffectUnitSight,
	NodeEffectUnitSpeed,
	NodeEffectBuildingSight,
	NodeEffectBuildingRecycleSouls,
	NodeEffectBuildingOverclockDuration,
	NodeEffectBuildingConstructionTime,
	NodeEffectBuildingConstructionCost,
};


UENUM(Blueprintable)
enum class ETransmutationNodeType : uint8
{
	TransmutationNodeTypeNone,
	TransmutationNodeSmall,
	TranmutationNodeBig,
};


UENUM(Blueprintable)
enum class ESoulsGainCostReason : uint8
{
	None,
	StructureGain,
	BaseIncome,
	NeutralCampReward,
	SoulBeaconReward,
	BuildingRecycled,

	BuildingConstruction,
	BuildingUpgrade,
	DebugIncome,

	TransmutationNodeRefund,
};


UENUM(Blueprintable)
enum ERemovingFluxMode : uint8
{
	None,
	RemoveFlux,
	RemoveFluxNode,
};


UENUM(Blueprintable)
enum EFluxModeState : uint8
{
	FMSNone,
	FMSMoveFluxNode,
	FMSMoveFluxNodeCnC,
	FMSRemoveFlux,
	FMSRemoveFluxNode,
};

UENUM(Blueprintable)
enum EFluxPowerScaling : uint8
{
	VeryLow,
	Low,
	Average,
	High
};

UENUM(Blueprintable)
enum class ESoundFamily : uint8
{
	Ambient,
	Effect,
	Music,
	UI
};

UENUM(Blueprintable)
enum class EFluxHoverInfoType : uint8
{
	FluxHoverInfoTypeNone,
	FluxHoverInfoTypeFluxHovered,
	FluxHoverInfoTypeFluxNodeHovered,
	FluxHoverInfoTypeFluxNodeMoving,
	FluxHoverInfoTypeFluxNodeMovingWithAllowed,
};

UENUM(Blueprintable)
enum class EVictoryPointReason : uint8
{
	EVPNone,
	MainBuildingCaptured,
	MainBuildingLost,
	BossKilled,
	PandemoniumMBIncome,
	DefaultBases,
	VictoryPointCheat
};