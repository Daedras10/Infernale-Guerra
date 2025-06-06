// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/BattleManagerComponent.h"

#include "Enums/Enums.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/AmalgamVisualisationManager.h"
#include "Manager/UnitActorManager.h"
#include "Structs/SimpleStructs.h"
#include "UnitAsActor/NiagaraUnitAsActor.h"


FBattleInfoCode::FBattleInfoCode()
	: BattlePositionAttackerWorld(FVector2D(0.f, 0.f)), BattlePositionTargetWorld(FVector2D(0.f, 0.f)),
	AttackerUnitType(EEntityType::EntityTypeNone), TargetUnitType(EEntityType::EntityTypeNone),
	UnitTargetTypeTarget(EUnitTargetType::UTargetNone), AttackType(EAttackType::Single)
{
}

// Sets default values for this component's properties
UBattleManagerComponent::UBattleManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UBattleManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	AUnitActorManager* UnitActorManagerPtr = Cast<AUnitActorManager>(GetOwner());
	UnitActorManager = TWeakObjectPtr<AUnitActorManager>(UnitActorManagerPtr);
	GameMode = Cast<AGameModeInfernale>(UGameplayStatics::GetGameMode(GetWorld()));
	AmalgamVisualisationManager = Cast<AAmalgamVisualisationManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass()));
}

// bool UBattleManagerComponent::SearchForBattle(FVector2D BattlePositionWorld, AUnitBattle*& OutBattle)
// {
// 	auto ClosestBattleDistance = BattleSearchRadius * 2;
// 	for (const auto Battle : Battles)
// 	{
// 		const auto Distance = FVector2D::Distance(Battle->GetLocation(), BattlePositionWorld);
// 		
// 		if (Distance > BattleDefaultRadius) continue;
// 		if (Distance > ClosestBattleDistance) continue;
//
// 		ClosestBattleDistance = Distance;
// 		OutBattle = Battle;
// 	}
// 	
// 	return (ClosestBattleDistance <= BattleDefaultRadius);
// }
//
// void UBattleManagerComponent::PrepareVFXForLastBattles()
// {
// 	for (const auto BattleInfos : BattleInfosSinceLastCheck)
// 	{
// 		// Battle->PrepareVFX();
// 	}
//
// 	BattleInfosSinceLastCheck.Empty();
// }

void UBattleManagerComponent::DoVFXMulticast_Implementation(FBattleInfo BattleInfo)
{
	// float Smoke;
	// if (BattleInfo.AttackerSmoke > 0)
	// {
	// 	if (BattleInfo.TargetSmoke > 0) Smoke = FMath::Min(BattleInfo.AttackerSmoke, BattleInfo.TargetSmoke);
	// 	else Smoke = BattleInfo.AttackerSmoke;
	// }
	// else if (BattleInfo.TargetSmoke > 0) Smoke = BattleInfo.TargetSmoke;
	// else Smoke = 800.0f;
	
    DoVFXonBP(BattleInfo);
}

void UBattleManagerComponent::DoDeathVFXMulticast_Implementation(FDeathInfo DeathInfo)
{
	DoDeathVFXonBP(DeathInfo);
}

void UBattleManagerComponent::AtPosBattleInfo(FBattleInfoCode BattleInfoCode)
{
	// For now, tests with vfx only on server
	const float Random = FMath::FRand();
	switch (BattleInfoCode.AttackType)
	{
	case EAttackType::Single:
		if (Random > VFXChances) return;
		break;
	case EAttackType::Multiple:
		if (Random > VFXZoneChances) return;
		break;

	default:
		break;
	}

	/* Player controllers */
	const auto Radius = AmalgamVisualisationManager->GetRadius();
	
	auto PlayerControllers = GameMode->GetPlayerControllers();
	for (auto PlayerController : PlayerControllers)
	{
		const auto CenterPos = PlayerController->GetCameraCenterPoint();
		const auto CenterPos2D = FVector2D(CenterPos.X, CenterPos.Y);
		const auto Distance = FVector2D::Distance(CenterPos2D, BattleInfoCode.BattlePositionAttackerWorld);
		if (Distance > Radius) continue;

		FBattleInfo BattleInfo = BattleInfoCodeToBattleInfo(BattleInfoCode);
		PlayerController->DoVfx(BattleInfo);
	}
}

void UBattleManagerComponent::AtPosDeathInfo(FDeathInfo DeathInfo)
{
	// For now, tests with vfx only on server
	const float Random = FMath::FRand();
	if (Random > VFXChances) return;
	
	/* Player controllers */
	const auto Radius = AmalgamVisualisationManager->GetRadius();

	auto PlayerControllers = GameMode->GetPlayerControllers();
	for (auto PlayerController : PlayerControllers)
	{
		const auto CenterPos = PlayerController->GetCameraCenterPoint();
		const auto CenterPos2D = FVector2D(CenterPos.X, CenterPos.Y);
		const auto Distance = FVector2D::Distance(CenterPos2D, DeathInfo.DeathPositionWorld);
		if (Distance > Radius) continue;

		PlayerController->DoDeathVfx(DeathInfo);
	}
}

void UBattleManagerComponent::DoVFXBP(FBattleInfo BattleInfo)
{
	// float Smoke;
	// if (BattleInfo.AttackerSmoke > 0)
	// {
	// 	if (BattleInfo.TargetSmoke > 0) Smoke = FMath::Min(BattleInfo.AttackerSmoke, BattleInfo.TargetSmoke);
	// 	else Smoke = BattleInfo.AttackerSmoke;
	// }
	// else if (BattleInfo.TargetSmoke > 0) Smoke = BattleInfo.TargetSmoke;
	// else Smoke = 800.0f;
	
	DoVFXonBP(BattleInfo);
}

void UBattleManagerComponent::DoDeathVFXBP(FDeathInfo DeathInfo)
{
	DoDeathVFXonBP(DeathInfo);
}

FBattleInfo UBattleManagerComponent::BattleInfoCodeToBattleInfo(const FBattleInfoCode& BattleInfoCode)
{
	FBattleInfo BattleInfo = FBattleInfo();
	
	BattleInfo.BattlePositionAttackerWorld = BattleInfoCode.BattlePositionAttackerWorld;
	BattleInfo.BattlePositionTargetWorld = BattleInfoCode.BattlePositionTargetWorld;
	
	BattleInfo.AttackerUnitType = BattleInfoCode.AttackerUnitType;
	BattleInfo.TargetUnitType = BattleInfoCode.TargetUnitType;
	
	BattleInfo.AttackerOwner = BattleInfoCode.AttackerOwner;
	BattleInfo.TargetOwner = BattleInfoCode.TargetOwner;
	
	BattleInfo.UnitTargetTypeTarget = BattleInfoCode.UnitTargetTypeTarget;
	BattleInfo.AttackType = BattleInfoCode.AttackType;

	auto AttackerSmoke = -1.f;
	auto TargetSmoke = -1.f;

	if (BattleInfo.AttackerUnitType == EEntityType::EntityTypeBehemot ||
		BattleInfo.AttackerUnitType == EEntityType::EntityTypeGobborit ||
		BattleInfo.AttackerUnitType == EEntityType::EntityTypeGobborit)
	{
		auto Id = BattleInfoCode.AttackerID.AsNumber();
		auto Visual = AmalgamVisualisationManager->FindElementBP(Id);
		if (Visual != nullptr)
		{
			if (Visual->Element.IsValid())
			{
				auto NiagaraUnit = Cast<ANiagaraUnitAsActor>(Visual->Element.Get());
				TWeakObjectPtr<ANiagaraUnitAsActor> NiagaraUnitPtr = TWeakObjectPtr<ANiagaraUnitAsActor>(NiagaraUnit);
				if (NiagaraUnitPtr.IsValid()) AttackerSmoke = NiagaraUnit->GetBattleManagerSmoke();
			}
		}
		
	}
	if (BattleInfo.TargetUnitType == EEntityType::EntityTypeBehemot ||
		BattleInfo.TargetUnitType == EEntityType::EntityTypeGobborit ||
		BattleInfo.TargetUnitType == EEntityType::EntityTypeGobborit)
	{
		auto Id = BattleInfoCode.TargetID.AsNumber();
		auto Visual = AmalgamVisualisationManager->FindElementBP(Id);
		if (Visual != nullptr)
		{
			if (Visual->Element.IsValid())
			{
				auto NiagaraUnit = Cast<ANiagaraUnitAsActor>(Visual->Element.Get());
				TWeakObjectPtr<ANiagaraUnitAsActor> NiagaraUnitPtr = TWeakObjectPtr<ANiagaraUnitAsActor>(NiagaraUnit);
				if (NiagaraUnitPtr.IsValid()) TargetSmoke = NiagaraUnit->GetBattleManagerSmoke();
			}
		}
		
	}
	if (AttackerSmoke > 0)
	{
		if (TargetSmoke > 0) BattleInfo.Smoke = FMath::Min(AttackerSmoke, TargetSmoke);
		else BattleInfo.Smoke = AttackerSmoke;
	}
	else if (TargetSmoke > 0) BattleInfo.Smoke = TargetSmoke;
	else BattleInfo.Smoke = 800.0f;
	
	return BattleInfo;
}

void UBattleManagerComponent::OnLaunchGame()
{
	Started = true;
}

