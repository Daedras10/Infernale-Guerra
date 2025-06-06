// Fill out your copyright notice in the Description page of Project Settings.

#include "Mass/Amalgam/Observers/AmalgamKillObserver.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"
#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>
#include <Kismet/GameplayStatics.h>

// Grid
#include "Mass/Collision/SpatialHashGrid.h"

// Misc
#include <LD/LDElement/Boss.h>
#include "Component/ActorComponents/BattleManagerComponent.h"

UAmalgamKillObserver::UAmalgamKillObserver() : EntityQuery(*this)
{
	ObservedType = FAmalgamKillTag::StaticStruct();

	Operation = EMassObservedOperation::Add;

	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);

	bRequiresGameThreadExecution = true;
}

void UAmalgamKillObserver::ConfigureQueries()
{
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadOnly);
	
	EntityQuery.AddTagRequirement<FAmalgamKillTag>(EMassFragmentPresence::All);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamKillObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!VisualisationManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0)
			VisualisationManager = static_cast<AAmalgamVisualisationManager*>(OutActors[0]);
		else
			VisualisationManager = nullptr;

		check(VisualisationManager);
	}
	/*if (!FogManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFogOfWarManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0)
			FogManager = static_cast<AFogOfWarManager*>(OutActors[0]);
		else
			FogManager = nullptr;

		check(FogManager);
	}*/

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
			
			TMap<TWeakObjectPtr<ASoulBeacon>, int> RewardMap;
			TMap<TWeakObjectPtr<ABoss>, int> BossMap;

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				EAmalgamDeathReason DeathReason = StateFragView[Index].GetDeathReason();

				FMassEntityHandle Handle = Context.GetEntity(Index);
				FMassEntityHandle TargetHandle = TargetFragView[Index].GetTargetEntityHandle();

				GridCellEntityData* UnitData = nullptr;

				if (ASpatialHashGrid::Contains(Handle)) UnitData = ASpatialHashGrid::GetMutableEntityData(Handle);

				//HashGridCell* Cell = ASpatialHashGrid::GetCellRef(Handle);
				//TWeakObjectPtr<ASoulBeacon> Beacon = ASpatialHashGrid::IsInSoulBeaconRangeByCell(Cell);
				TWeakObjectPtr<ASoulBeacon> Beacon = ASpatialHashGrid::IsInSoulBeaconRange(Handle);
				if (Beacon != nullptr && DeathReason == EAmalgamDeathReason::Eliminated)
				{
					//Beacon->Reward(ESoulBeaconRewardType::RewardAmalgam);
					if (RewardMap.Contains(Beacon)) ++RewardMap[Beacon];
					else RewardMap.Add(Beacon, 1);
				}

				TWeakObjectPtr<ABoss> Boss = ASpatialHashGrid::IsInBossRange(Handle);
				if (Boss != nullptr && DeathReason == EAmalgamDeathReason::Sacrificed)
				{
					if (BossMap.Contains(Boss)) ++BossMap[Boss];
					else BossMap.Add(Boss, 1);
				}
				
				if (UnitData)
				{
					UnitData->AggroCount--;
					TriggerDeathVFX(UnitData->Location, UnitData->Owner, UnitData->EntityType, DeathReason, Beacon != nullptr);
				}

				/*if (FogManager->Contains(Handle))
					FogManager->RemoveMassEntityVision(Handle);*/

				ASpatialHashGrid::RemoveEntityFromGrid(Handle);
				VisualisationManager->RemoveFromMapP(Handle);

				if (DeathReason == EAmalgamDeathReason::Error)
					UE_LOG(LogTemp, Error, TEXT("Error caused amalgam death"));
			
			}

			TArray<TWeakObjectPtr<ASoulBeacon>> Keys;
			RewardMap.GenerateKeyArray(Keys);
			for (int32 BeaconIndex = 0; BeaconIndex < Keys.Num(); ++BeaconIndex)
			{
				TWeakObjectPtr<ASoulBeacon> Beacon = Keys[BeaconIndex];
				Beacon->RewardMultiple(ESoulBeaconRewardType::RewardAmalgam, RewardMap[Beacon]);
			}

			TArray<TWeakObjectPtr<ABoss>> BossKeys;
			BossMap.GenerateKeyArray(BossKeys);
			for (int32 BossIndex = 0; BossIndex < BossKeys.Num(); ++BossIndex)
			{
				TWeakObjectPtr<ABoss> Boss = BossKeys[BossIndex];
				Boss->AskAddToSpawnCharge(Boss->ChargePerKill * BossMap[Boss]);
				// Boss->AddToSpawnChargeServer(Boss->ChargePerKill * BossMap[Boss]);
			}

			if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, FString::Printf(TEXT("KillObserver : Cleared %d Entities"), Context.GetNumEntities()));
			Context.Defer().DestroyEntities(Context.GetEntities());
		});
}

void UAmalgamKillObserver::TriggerDeathVFX(FVector Location, FOwner UnitOwner, EEntityType UnitType, EAmalgamDeathReason DeathReason, bool IsInSoulBeaconRange)
{
	if (!BattleManager) GetBattleManager();

	if (BattleManager)
	{
		FDeathInfo DeathInfo;
	
		DeathInfo.UnitOwner = UnitOwner;
		DeathInfo.UnitType = UnitType;
		DeathInfo.DeathPositionWorld = FVector2D(Location.X, Location.Y);
		DeathInfo.DeathReason = DeathReason;
		DeathInfo.InSoulBeaconRange = IsInSoulBeaconRange;

		BattleManager->AtPosDeathInfo(DeathInfo);
	}
}

void UAmalgamKillObserver::GetBattleManager()
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitActorManager::StaticClass(), OutActors);

	if (OutActors.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("AmalgamKillObserver : Unable to find UnitActorManager, skipping execution."));
		return;
	}

	auto UnitActorManager = static_cast<AUnitActorManager*>(OutActors[0]);
	if (!UnitActorManager) return;

	BattleManager = UnitActorManager->GetBattleManagerComponent().Get();
}
