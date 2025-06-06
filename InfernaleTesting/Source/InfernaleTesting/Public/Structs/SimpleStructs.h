// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CheckBox.h"
#include "Enums/Enums.h"
#include "SimpleStructs.generated.h"


class UImage;
class UHorizontalBox;
class URichTextBlock;

USTRUCT(BlueprintType)
struct FVector2DInt
{
	GENERATED_BODY()
public:
	FVector2DInt();
	FVector2DInt(const FVector& Arg);
	FVector2DInt(const FVector2D& Arg);
public:
	UPROPERTY(BlueprintReadWrite) int X;
	UPROPERTY(BlueprintReadWrite) int Y;
};

USTRUCT(Blueprintable)
struct FOwner
{
	GENERATED_BODY()
public:
	FOwner();

	bool operator==(const FOwner& Other) const
	{
		return (Player == Other.Player) && (Team == Other.Team);
	}
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) EPlayerOwning Player;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ETeam Team;
};

USTRUCT(Blueprintable)
struct FBattleInfo
{
	GENERATED_BODY()
public:
	FBattleInfo();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D BattlePositionAttackerWorld;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D BattlePositionTargetWorld;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) EEntityType AttackerUnitType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) EEntityType TargetUnitType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FOwner AttackerOwner;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FOwner TargetOwner;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) EUnitTargetType UnitTargetTypeTarget;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TEnumAsByte<EAttackType> AttackType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float Smoke = 800;
};

USTRUCT(Blueprintable)
struct FDeathInfo
{
	GENERATED_BODY()
public:
	FDeathInfo();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D DeathPositionWorld;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) EEntityType UnitType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FOwner UnitOwner;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) TEnumAsByte<EAmalgamDeathReason> DeathReason;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool InSoulBeaconRange;
};

USTRUCT(BlueprintType)
struct FAmalgamPresence
{
	GENERATED_BODY()
public:
	FAmalgamPresence();

public:
	UPROPERTY(BlueprintReadWrite) TArray<FVector2DInt> Positions;
	UPROPERTY(BlueprintReadWrite) FString Type;
	UPROPERTY(BlueprintReadWrite) ETeam Team;
	
};

USTRUCT(Blueprintable)
struct FArrayOfAmalgamPresence
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite) TArray<FAmalgamPresence> AmalgamPresences;
};

USTRUCT(BlueprintType)
struct FBaseCaptureInfo
{
	GENERATED_BODY()
	FBaseCaptureInfo();
public:
	UPROPERTY(BlueprintReadWrite) FVector2DInt Position;
	UPROPERTY(BlueprintReadWrite) FString Name;
	UPROPERTY(BlueprintReadWrite) ETeam Team;
	
};

USTRUCT(BlueprintType)
struct FBossKillInfo
{
	GENERATED_BODY()
public:
	FBossKillInfo();

public:
	UPROPERTY(BlueprintReadWrite) FVector2DInt Position;
	UPROPERTY(BlueprintReadWrite) ETeam CaptureTeam;
};


USTRUCT(BlueprintType)
struct FDataGathererPlayerInfo
{
	GENERATED_BODY()
public:
	FDataGathererPlayerInfo();

public:
	UPROPERTY(BlueprintReadWrite) FOwner Owner; // done

	UPROPERTY(BlueprintReadWrite) int NumberOfBases; // done
	UPROPERTY(BlueprintReadWrite) int BossesKilled; // done
	UPROPERTY(BlueprintReadWrite) int BossesSummoned = 0; // l8r

	UPROPERTY(BlueprintReadWrite) int SmallCampsKilled; // done
	UPROPERTY(BlueprintReadWrite) int BigCampsKilled; // done

	UPROPERTY(BlueprintReadWrite) TMap<FString, int> BuildingsCountPerType; // done
	UPROPERTY(BlueprintReadWrite) int SoulsInReserve; // done
	UPROPERTY(BlueprintReadWrite) int SoulsFromIncome; // done
	UPROPERTY(BlueprintReadWrite) int SoulsFromMonsters; // done
	UPROPERTY(BlueprintReadWrite) int SoulsFromBeacons;
	
	UPROPERTY(BlueprintReadWrite) int DominationPoints; // done
	// UPROPERBlueprintReadWriteTY() TArray<FFluxInfo> ActiveFluxes;

	UPROPERTY(BlueprintReadWrite) TArray<FString> ActiveTransmutation; // done
	
};

USTRUCT(BlueprintType)
struct FPlayerSnapshot
{
	GENERATED_BODY()
public:
	FPlayerSnapshot();

public:
	UPROPERTY(BlueprintReadWrite) float Time; // done
	UPROPERTY(BlueprintReadWrite) FString TimeMMSS; // done
 
	UPROPERTY(BlueprintReadWrite) TArray<FDataGathererPlayerInfo> PlayerInfos;

	UPROPERTY(BlueprintReadWrite) int AmalgamsOnMap; // done
	UPROPERTY(BlueprintReadWrite) int AmalgamsKilledSinceLast = 0;
	// UPROPERTY(BlueprintReadWrite) TMap<FVector, int> AmalgamsKilledInBeaconZoneSinceLast;

	UPROPERTY(BlueprintReadWrite) TArray<FBaseCaptureInfo> BasesInfos; // done
	UPROPERTY(BlueprintReadWrite) TArray<FBossKillInfo> BossesKilledInfo; // done

	UPROPERTY(BlueprintReadWrite) TArray<FAmalgamPresence> AmalgamPositionsAndType; // done
	UPROPERTY(BlueprintReadWrite) TArray<FVector2DInt> CombatPositions;

	FString GetFormattedTime() const
	{
		return FString::Printf(TEXT("%02d:%02d"), FMath::FloorToInt(Time / 60), FMath::FloorToInt(Time) % 60);
	}
};


USTRUCT(Blueprintable)
struct FTransmutationEffects
{
	GENERATED_BODY()
public:
	FTransmutationEffects();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ENodeEffect NodeEffect;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ValueFlat = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ValuePercent = 1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ValueFlatCurse = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ValuePercentCurse = 1;
};

USTRUCT(Blueprintable)
struct FTransmutationSimpleEffects
{
	GENERATED_BODY()
public:
	FTransmutationSimpleEffects();
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ENodeEffect NodeEffect;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int Level = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UCurveFloat* ValuePercentCurve;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ValuePercent = 0;
};


USTRUCT(Blueprintable)
struct FNodeEffect
{
	GENERATED_BODY()
public:
	FNodeEffect();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ENodeEffect NodeEffect;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float Value;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool IsPercentage = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool IsCurse = false;
};

USTRUCT(Blueprintable)
struct FSimpleNodeEffect
{
	GENERATED_BODY()
public:
	FSimpleNodeEffect();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ENodeEffect NodeEffect;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UCurveFloat* ValuePercentageCurve;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UCurveFloat* PriceCurve;
};


USTRUCT(Blueprintable)
struct FTransmutationNodeVisualInfo
{
	GENERATED_BODY()
public:
	FTransmutationNodeVisualInfo();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D Position;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ETransmutationNodeType TransmutationNodeType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UTexture2D* Icon;
};


USTRUCT(Blueprintable)
struct FCostStruct
{
	GENERATED_BODY()
public:
	FCostStruct();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float BaseValue = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ValuePerUnit = 0;
};


USTRUCT(Blueprintable)
struct FTransmutationSettings
{
	GENERATED_BODY()
public:
	FTransmutationSettings();

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D BigNodeSize = FVector2D(3, 3);
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D SmallNodeSize = FVector2D(1, 1);

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D CellSize = FVector2D(35, 35);
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D StartOffset = FVector2D(0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FCostStruct BigNodeCost;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FCostStruct SmallNodeCost;
	
	// UPROPERTY(BlueprintReadWrite, EditAnywhere) FCostStruct BigNodeCost;
	// UPROPERTY(BlueprintReadWrite, EditAnywhere) FCostStruct SmallNodeCost;
};


USTRUCT(Blueprintable)
struct FBuildingRessourceGain
{
	GENERATED_BODY()
public:
	FBuildingRessourceGain();
	
};


USTRUCT(Blueprintable)
struct FSoulsGainCostValues
{
	GENERATED_BODY()
public:
	FSoulsGainCostValues();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ESoulsGainCostReason SoulsGainReason = ESoulsGainCostReason::None;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float ValuePerBuilding = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float BaseValue = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float Time = 1.0f;
};


USTRUCT(Blueprintable)
struct FPathStruct
{
	GENERATED_BODY()
public:
	FPathStruct();
	FPathStruct(FVector InPathPoint, bool InIsReal);

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector PathPoint;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool IsReal;
};


USTRUCT(Blueprintable)
struct FActorVector
{
	GENERATED_BODY()
public:
	FActorVector();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector Vector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) AActor* Actor;
};

USTRUCT(Blueprintable)
struct FFloatTeamStruct
{
	GENERATED_BODY()
public:
	FFloatTeamStruct();
	
public:
	UPROPERTY()	ETeam Team;
	UPROPERTY()	float Value;
};

USTRUCT(Blueprintable)
struct FTotalSoulsGainForLastSnapshotStruct
{
	GENERATED_BODY()
public:
	FTotalSoulsGainForLastSnapshotStruct();
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)	TArray<FFloatTeamStruct> TotalSoulsGainForLastSnapshot;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)	TArray<FFloatTeamStruct> AveragedTotalSoulsGainForLastSnapshot;
};

USTRUCT(BlueprintType)
struct FPlayerChartData
{
	GENERATED_BODY()
public:
	FPlayerChartData();
	FPlayerChartData(const TArray<float>& InValues, const TArray<FString>& InXAxisLabels);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<float> Values;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FString> XAxisLabels;
};

USTRUCT(BlueprintType)
struct FBaseCaptureArray
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FBaseCaptureInfo> BasesInfosArray;
	
};


USTRUCT(Blueprintable)
struct FDebugSettings
{
	GENERATED_BODY()
public:
	FDebugSettings();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool ShowSoundDebug = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool ShowVFXDebug = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool ShowHoverTypeDebug = false;
};

USTRUCT(Blueprintable)
struct FAttackHitInfo
{
	GENERATED_BODY()
public:
	FAttackHitInfo();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector HitLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<ETeam> HitTeams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HitDamage;
};

USTRUCT(Blueprintable)
struct FArrayOfUObjects
{
	GENERATED_BODY()
public:
	FArrayOfUObjects();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UCheckBox* CheckBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) URichTextBlock* RichTextBlock;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UHorizontalBox* HorizontalBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UImage* Image;
};

USTRUCT(BlueprintType)
struct FSoundType
{
	GENERATED_BODY()
public:
	FSoundType();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SoundName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* Sound = FindObject<USoundBase>(nullptr, TEXT("/Script/Engine.SoundWave'/Game/_Draft/Damien/Sound/SecretSound.SecretSound'"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESoundFamily SoundFamily = ESoundFamily::Effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Volume = 1.f;

	
};

USTRUCT(Blueprintable)
struct FSoundStruct
{
	GENERATED_BODY()
public:
	FSoundStruct();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TypeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoundType> Sounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GlobalSoundTypeVolume = 1.f;
};

USTRUCT(BlueprintType)
struct FStringPlayerChartData
{
	GENERATED_BODY()
public:
	FStringPlayerChartData();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FString String;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FPlayerChartData PlayerChartData;
};


USTRUCT(Blueprintable)
struct FPlayerInfo
{
	GENERATED_BODY()
public:
	FPlayerInfo();

	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int PlayerID;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FString PlayerName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FOwner PlayerOwnerInfo;
	//UPROPERTY(BlueprintReadWrite, EditAnywhere) int PlayerAffinityWithRTS;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) APlayerController* PlayerController;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool PlayerIsReady = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool isInRoom = true;
};

USTRUCT(Blueprintable)
struct FVictoryInfo
{
	GENERATED_BODY()
public:
	FVictoryInfo();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int VictoryPoints;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int VictoryRank;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int RankPos;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool PlayerAlive = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerName;
};

USTRUCT(Blueprintable)
struct FVictoryInfoMap
{
	GENERATED_BODY()
public:
	FVictoryInfoMap();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<ETeam, FVictoryInfo> VictoryInfoMap;
};

USTRUCT(Blueprintable)
struct FDeadTeam
{
	GENERATED_BODY()
public:
	FDeadTeam();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<ETeam> DeadTeams;
};

USTRUCT(Blueprintable)
struct FTimeRemaining
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TimeRemaining = 0;
};

USTRUCT(Blueprintable)
struct FVictoryPoints
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float VictoryPointsNature = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float VictoryPointsTeam1 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float VictoryPointsTeam2 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float VictoryPointsTeam3 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float VictoryPointsTeam4 = 0;

	int TeamsPresent;
};