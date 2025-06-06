// Fill out your copyright notice in the Description page of Project Settings.


#include "Structs/SimpleStructs.h"

FVector2DInt::FVector2DInt()
{
	X = 0;
	Y = 0;
}

FVector2DInt::FVector2DInt(const FVector& Arg)
{
	X = FMath::RoundToInt(Arg.X);
	Y = FMath::RoundToInt(Arg.Y);
}

FVector2DInt::FVector2DInt(const FVector2D& Arg)
{
	X = FMath::RoundToInt(Arg.X);
	Y = FMath::RoundToInt(Arg.Y);
}

FOwner::FOwner() : Player(EPlayerOwning::Nature), Team(ETeam::NatureTeam)
{
}

FBattleInfo::FBattleInfo() : 
	BattlePositionAttackerWorld(FVector2D(0.f, 0.f)), BattlePositionTargetWorld(FVector2D(0.f, 0.f)),
	AttackerUnitType(EEntityType::EntityTypeNone), TargetUnitType(EEntityType::EntityTypeNone), AttackerOwner(FOwner()),
	TargetOwner(FOwner()), UnitTargetTypeTarget(EUnitTargetType::UTargetNone), AttackType(EAttackType::Single), Smoke(800)
{
}

FDeathInfo::FDeathInfo() :
	DeathPositionWorld(FVector2D(0.f, 0.f)), UnitType(EEntityType::EntityTypeNone), DeathReason(),
	InSoulBeaconRange(false)
{
}

FAmalgamPresence::FAmalgamPresence() :
	Positions(), Type(""), Team(ETeam::NatureTeam)
{
}

FBaseCaptureInfo::FBaseCaptureInfo(): Position(), 
	Name(""), Team(ETeam::NatureTeam)
{
}

FBossKillInfo::FBossKillInfo(): Position(), CaptureTeam()
{
}

FDataGathererPlayerInfo::FDataGathererPlayerInfo(): NumberOfBases(0), BossesKilled(0), SmallCampsKilled(0),
                                                    BigCampsKilled(0),
                                                    SoulsInReserve(0),
                                                    SoulsFromIncome(0),
                                                    SoulsFromMonsters(0),
                                                    SoulsFromBeacons(0),
                                                    DominationPoints(0)
{
}

FPlayerSnapshot::FPlayerSnapshot(): Time(0), AmalgamsOnMap(0), AmalgamsKilledSinceLast(0),
	/*AmalgamsKilledInBeaconZoneSinceLast(),*/ BasesInfos(), BossesKilledInfo(), AmalgamPositionsAndType(),
	CombatPositions()
{
}

FTransmutationEffects::FTransmutationEffects(): NodeEffect()
{
}

FTransmutationSimpleEffects::FTransmutationSimpleEffects(): NodeEffect(ENodeEffect::NodeEffectNone), ValuePercentCurve(nullptr)
{
}

FNodeEffect::FNodeEffect(): NodeEffect(ENodeEffect::NodeEffectNone), Value(0)
{
}

FSimpleNodeEffect::FSimpleNodeEffect(): NodeEffect(ENodeEffect::NodeEffectNone), ValuePercentageCurve(nullptr), PriceCurve(nullptr)
{
}

FTransmutationNodeVisualInfo::FTransmutationNodeVisualInfo(): Position(FVector2D(0.f, 0.f)), TransmutationNodeType(ETransmutationNodeType::TransmutationNodeTypeNone), Icon(nullptr)
{
}

FCostStruct::FCostStruct()
{
}

FTransmutationSettings::FTransmutationSettings()
{
}

FBuildingRessourceGain::FBuildingRessourceGain()
{
}

FSoulsGainCostValues::FSoulsGainCostValues()
{
}

FPathStruct::FPathStruct(): PathPoint(FVector(0.f, 0.f, 0.f)), IsReal(false)
{
}

FPathStruct::FPathStruct(FVector InPathPoint, bool InIsReal) : PathPoint(InPathPoint), IsReal(InIsReal)
{
}

FActorVector::FActorVector(): Vector(FVector(0.f, 0.f, 0.f)), Actor(nullptr)
{
}

FFloatTeamStruct::FFloatTeamStruct(): Team(ETeam::NatureTeam), Value(0)
{
}

FTotalSoulsGainForLastSnapshotStruct::FTotalSoulsGainForLastSnapshotStruct()
{
}

FPlayerChartData::FPlayerChartData()
{
}

FPlayerChartData::FPlayerChartData(const TArray<float>& InValues, const TArray<FString>& InXAxisLabels)
	: Values(InValues), XAxisLabels(InXAxisLabels)
{
}

FDebugSettings::FDebugSettings()
{
}

FAttackHitInfo::FAttackHitInfo(): HitLocation(FVector(0.f, 0.f, 0.f)), HitDamage(0)
{
}

FArrayOfUObjects::FArrayOfUObjects(): CheckBox(nullptr), RichTextBlock(nullptr), HorizontalBox(nullptr), Image(nullptr)
{
}

FSoundType::FSoundType()
{
}

FSoundStruct::FSoundStruct()
{
}

FStringPlayerChartData::FStringPlayerChartData()
{
}

FPlayerInfo::FPlayerInfo(): PlayerID(0), PlayerController(nullptr)
{
}
