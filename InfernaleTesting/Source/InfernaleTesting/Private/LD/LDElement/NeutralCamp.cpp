// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/LDElement/NeutralCamp.h"

#include "Component/ActorComponents/AttackComponent.h"
#include "Component/ActorComponents/DamageableComponent.h"
#include "Component/ActorComponents/EffectAfterDelayComponent.h"
#include "Component/ActorComponents/UnitActorGridComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "DataAsset/AttacksDataAsset.h"
#include "DataAsset/NeutralCampDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Manager/UnitActorManager.h"
#include <Mass/Collision/SpatialHashGrid.h>

#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "LeData/DataGathererActor.h"

ANeutralCamp::ANeutralCamp()
{
	EffectAfterDelayComponent = CreateDefaultSubobject<UEffectAfterDelayComponent>(TEXT("EffectAfterDelayComponent"));
	DamageableComponent = CreateDefaultSubobject<UDamageableComponent>(TEXT("DamageableComponent"));
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	
	PrimaryActorTick.bCanEverTick = true;
	LDElementType = ELDElementType::LDElementNeutralCampType;
}

UDamageableComponent* ANeutralCamp::GetDamageableComponent() const
{
	return DamageableComponent;
}

FOwner ANeutralCamp::GetOwner()
{
	auto OwnerToReturn = FOwner();
	OwnerToReturn.Player = EPlayerOwning::Nature;
	OwnerToReturn.Team = ETeam::NatureTeam;
	return OwnerToReturn;
}

float ANeutralCamp::DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner)
{
	if (!HasAuthority()) return 0.f;
	return DamageableComponent->DamageHealthOwner(DamageAmount, bDamagePercent, DamageOwner);
}

float ANeutralCamp::GetTargetableRange()
{
	return TargetableRange;
}

bool ANeutralCamp::IsAttackableBy(ETeam Team)
{
	return IsAlive;
}

EEntityType ANeutralCamp::GetEntityType() const
{
	return EEntityType::EntityTypeNeutralCamp;
}

void ANeutralCamp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority()) return;
	if (IsRespawning)
	{
		RespawnCurrentTime -= DeltaTime;
		if (RespawnCurrentTime <= 0.f)
		{
			RespawnCurrentTime = 0.f;
			IsRespawning = false;
		}
		VisualRespawnTimeBP(RespawnCurrentTime, RespawnMaxTime);
	}

	if (bDebugReplicationSpecific) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, FString::Printf(TEXT("ANeutralCamp::Tick > bShouldReplicateDamage %s"), bShouldReplicateDamage ? TEXT("true") : TEXT("false")));
	}
	if (bShouldReplicateDamage)
	{
		DamageReplicationTime += DeltaTime;
		if (DamageReplicationTime >= TimeBetweenDamageReplication)
		{
			bShouldReplicateDamage = false;
			DamageReplicationTime = 0.f;
			bTickRequieredForDamage = false;
			EnableTickForActor(false);

			VisualDamagedMulticast(NewHealthToReplicate, DamageAmountToReplicate, PercentToReplicate);
		}
	}
	
}

float ANeutralCamp::GetOffsetRange()
{
	return 0;
}

float ANeutralCamp::GetAttackOffsetRange()
{
	return AttackOffsetRange;
}

void ANeutralCamp::EnableTickForActor(bool bEnable)
{
	const bool ToEnable = bTickRequieredForDamage || bTickRequieredForRespawn || bEnable;
	SetActorTickEnabled(true);
	//SetActorTickEnabled(ToEnable);
}

void ANeutralCamp::BeginPlay()
{
	if (bIsDebugTarget)
		UE_DEBUG_BREAK();

	Super::BeginPlay();
	EnableTickForActor(false);
	SyncDataAsset();

	Type = EUnitTargetType::UTargetNeutralCamp;
	
	if (!HasAuthority()) return;
	IsAlive = bStartSpawned;

	EffectAfterDelayComponent->FinishedDelegate.AddDynamic(this, &ANeutralCamp::OnRespanwTimerFinished);
	EffectAfterDelayComponent->TimeRemainingDelegate.AddDynamic(this, &ANeutralCamp::OnTimeRemaining);

	DamageableComponent->HealthDepeltedOwner.AddDynamic(this, &ANeutralCamp::OnDeath);
	DamageableComponent->Damaged.AddDynamic(this, &ANeutralCamp::OnDamaged);

	DamageableComponent->SetMaxHealth(MaxHealth, true, true, false, MaxHealth);

	DataGathererActor = Cast<ADataGathererActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ADataGathererActor::StaticClass()));

	
	auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameMode) OnLauchGame();
	else
	{
		GameMode->PreLaunchGame.AddDynamic(this, &ANeutralCamp::OnPreLauchGame);
		GameMode->LaunchGame.AddDynamic(this, &ANeutralCamp::OnLauchGame);
	}
	
	AttackPerformed.AddDynamic(this, &ANeutralCamp::OnAttackPerformed);	

	AttackComponent->AttackReadyDelegate.AddDynamic(this, &ANeutralCamp::CallOnAttackReady);
}

void ANeutralCamp::RegisterLDElement()
{
	// Nothing for now, refactor latter
}

void ANeutralCamp::SyncDataAsset()
{
	if (!bUseDataAsset) return;
	if (!NeutralCampDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("NeutralCamp Error : \n\t No NeutralCamp Data Asset"));
		return;
	}

	NeutralCampRewards = NeutralCampDataAsset->RewardData;
	MaxHealth = NeutralCampDataAsset->MaxHealth;
	RespawnTime = NeutralCampDataAsset->RespawnTime;
	AttackOffsetRange = NeutralCampDataAsset->AttackOffsetRange;
	TargetableRange = NeutralCampDataAsset->TargetableRange;
}

void ANeutralCamp::StartRespawn()
{
	EffectAfterDelayComponent->SetTime(RespawnTime);
	EffectAfterDelayComponent->StartFromBegining();
}

void ANeutralCamp::DeathEffects(FOwner Depleter)
{
	const auto World = GetWorld();
	const auto GameModeInfernale = Cast<AGameModeInfernale>(World->GetAuthGameMode());
	const auto PlayerStateInfernale = GameModeInfernale->GetPlayerState(Depleter.Player);
	if (!PlayerStateInfernale.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("PlayerStateInfernale not found in ANeutralCamp::DeathEffects"));
		return;
	}
	for (auto Effect : NeutralCampRewards)
	{
		switch (Effect.RewardType) {
		case ENeutralCampReward::ENeutralMobRewardNone:
			break;
		case ENeutralCampReward::ENeutralMobRewardSouls:
			PlayerStateInfernale->GetEconomyComponent()->AddSouls(this, ESoulsGainCostReason::NeutralCampReward, Effect.RewardValue);
			if (DataGathererActor != nullptr)
			{
				DataGathererActor->AddMonsterSoulsToOwner(Depleter, Effect.RewardValue, CampType);
			}
			else {GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("DataGathererActor not found in ANeutralCamp::DeathEffects"));}
			break;
		case ENeutralCampReward::ENeutralMobRewardVision:
			break;
		case ENeutralCampReward::ENeutralMobRewardDomination:
			GameModeInfernale->GetVictoryManagerComponent()->AddPointsOnBossDeath(Depleter,Effect.RewardValue);
			break;
		}
	}
	OnDeathEffectVFXTrigger(GetActorLocation(), Depleter);
}

void ANeutralCamp::Respawn()
{
	SpawnInGrid();
	DamageableComponent->HealHealth(1.f, true);
	VisualRespawnMulticast();
	IsAlive = true;
}

void ANeutralCamp::KillInGrid()
{
	//UnitActorManager->GetGridComponent()->RemoveLDElement(TWeakObjectPtr<ALDElement>(this));
	WasAddedToGrid = !ASpatialHashGrid::RemoveLDElementFromGrid(GetActorLocation(), this);
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString("NeutralCamp Killed WasAddedToGrid : ") + FString::Printf(TEXT("%d"), WasAddedToGrid));
	// UE_LOG(LogTemp, Warning, TEXT("NeutralCamp Killed WasAddedToGrid : %d"), WasAddedToGrid);
	LDElementRemoved.Broadcast(this);
}

void ANeutralCamp::SpawnInGrid()
{
	//UnitActorManager->GetGridComponent()->AddLDElement(TWeakObjectPtr<ALDElement>(this));
	WasAddedToGrid = ASpatialHashGrid::AddLDElementToGrid(GetActorLocation(), this);
	// GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString("NeutralCamp Spawned WasAddedToGrid : ") + FString::Printf(TEXT("%d"), WasAddedToGrid));
	// UE_LOG(LogTemp, Warning, TEXT("NeutralCamp Spawned WasAddedToGrid : %d"), WasAddedToGrid);
}

void ANeutralCamp::TryToGetUnitActorManager()
{
	const auto Success = UFunctionLibraryInfernale::TryGetUnitActorManager(UnitActorManager, UnitActorManagerClass);
	if (!Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("UnitActorManager not found in ANeutralCamp::BeginPlay"));
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ANeutralCamp::TryToGetUnitActorManager, 0.05f, false);
		return;
	}

	if (bStartSpawned)
	{
		Respawn();
		return;
	}
	StartRespawn();
}

void ANeutralCamp::OnRespanwTimerFinished(AActor* _)
{
	Respawn();
	RespawnFinishedMulticast();
	IsRespawning = false;
}

void ANeutralCamp::OnTimeRemaining(float TimeRemaining, float TotalTime)
{
	if (!IsRespawning)
	{
		IsRespawning = true;
		LastReplication = TimeRemaining;
		StartRespawningMulticast(TimeRemaining, TotalTime);
	}

	VisualRespawnTimeBP(TimeRemaining, TotalTime);

	if (FMath::Abs(LastReplication - TimeRemaining) < 1.f) return;
	VisualRespawnTimeMulticast(TimeRemaining, TotalTime);
	LastReplication = TimeRemaining;
}

void ANeutralCamp::OnDeath(AActor* _, FOwner Depleter)
{
	IsAlive = false;
	VisualDeathMulticast(Depleter);
	KillInGrid();
	DeathEffects(Depleter);
	if (bCanRespawn) StartRespawn();
}

void ANeutralCamp::OnDamaged(AActor* _, float NewHealth, float DamageAmount)
{
	if (!bShouldReplicateDamage)
	{
		if (bDebugReplicationSpecific)
		{
			UE_LOG(LogTemp, Warning, TEXT("ANeutralCamp::OnDamaged : Activate replication"));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ANeutralCamp::OnDamaged : Activate replication")));
		}
		bShouldReplicateDamage = true;
		DamageReplicationTime = 0.f;
		
		bTickRequieredForDamage = true;
		EnableTickForActor(true);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ANeutralCamp::OnDamaged : Was already replicating")));
	}

	NewHealthToReplicate = NewHealth;
	DamageAmountToReplicate = DamageAmount;
	PercentToReplicate = NewHealth / MaxHealth;
	VisualDamagedBP(NewHealth, DamageAmount, PercentToReplicate);
}

void ANeutralCamp::InitAttacks()
{
	if (!AttacksDataAsset) return;

	AttackComponent->SetAttacks(AttacksDataAsset->Attacks);
	AttackComponent->SetStarted(true);
}

void ANeutralCamp::OnPreLauchGame()
{
	TryToGetUnitActorManager();
}

void ANeutralCamp::OnLauchGame()
{
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ANeutralCamp::InitAttacks, 0.1f, false);
}

void ANeutralCamp::RespawnFinishedMulticast_Implementation()
{
	IsRespawning = false;
	bTickRequieredForRespawn = false;
	EnableTickForActor(false);
}

void ANeutralCamp::StartRespawningMulticast_Implementation(float TimeRemaining, float TotalTime)
{
	IsRespawning = true;
	bTickRequieredForRespawn = true;
	EnableTickForActor(true);
	RespawnCurrentTime = TimeRemaining;
	RespawnMaxTime = TotalTime;
}

bool ANeutralCamp::OnAttackReady(FAttackStruct AttackStruct)
{
	if (!WasAddedToGrid) return false;
	
	if (bIsDebugTarget)
	{
		UE_DEBUG_BREAK();
		bIsDebugTarget = false;
	}
	//GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Magenta, "OnAttackReady : " + GetName());

	if (NeutralCampDataAsset->bUseAOEAttack)
	{
		auto AttackedHandles = ASpatialHashGrid::FindEntitiesInRange(GetActorLocation(), AttackStruct.AttackRange, AttackStruct.AttackAngle, GetActorForwardVector(), FMassEntityHandle(0, 0));
		TArray<FMassEntityHandle> Keys;
		AttackedHandles.GenerateKeyArray(Keys);
		if (AttackedHandles.Num() == 0) return false;

		TArray<ETeam> Teams;
		for (auto Key : Keys)
		{
			const auto Data = ASpatialHashGrid::GetMutableEntityData(Key);
			if (!Data) continue;
			Data->DamageEntity(AttackStruct.AttackDamage);
			Teams.AddUnique(Data->Owner.Team);
		}
		if (bDebugTarget) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Damaged %d entities for : %d"), Keys.Num(), AttackStruct.AttackDamage));
		FAttackHitInfo AttackHitInfo;
		AttackHitInfo.HitDamage = AttackStruct.AttackDamage;
		AttackHitInfo.HitLocation = GetActorLocation();
		AttackHitInfo.HitTeams = Teams;
		AttackPerformed.Broadcast(AttackHitInfo);
		return true;
	}

	FMassEntityHandle AttackedHandle = ASpatialHashGrid::FindClosestEntity(GetActorLocation(), AttackStruct.AttackRange, AttackStruct.AttackAngle, GetActorForwardVector(), FMassEntityHandle(0, 0), ETeam::NatureTeam);

	if (AttackedHandle.Index == 0) return false;
	if (!ASpatialHashGrid::Contains(AttackedHandle))
	{
		//if (bDebugTarget) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("No entity found to attack"));
		return false;
	}

	const auto Data = ASpatialHashGrid::GetMutableEntityData(AttackedHandle);
	if (!Data) return false;

	Data->DamageEntity(AttackStruct.AttackDamage);
	if (bDebugTarget) GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Damaged entity for : %d"), AttackStruct.AttackDamage));
	TArray<ETeam> Teams;
	Teams.AddUnique(Data->Owner.Team);
	FAttackHitInfo AttackHitInfo;
	AttackHitInfo.HitDamage = AttackStruct.AttackDamage;
	AttackHitInfo.HitLocation = GetActorLocation();
	AttackHitInfo.HitTeams = Teams;
	AttackPerformed.Broadcast(AttackHitInfo);
	return true;

	//return UnitActorManager->Attack(this, AttackStruct, EAttackerType::AttackerTypeNeutralCamp);
}

void ANeutralCamp::CallOnAttackReady(FAttackStruct AttackStruct)
{
	OnAttackReady(AttackStruct);
}

void ANeutralCamp::VisualDamagedMulticast_Implementation(float NewHealth, float DamageAmount, float Percent)
{
	if (HasAuthority()) return;
	VisualDamagedBP(NewHealth, DamageAmount, Percent);
}

void ANeutralCamp::VisualRespawnMulticast_Implementation()
{
	VisualRespawnBP();
	OnCampRespawnVFXTrigger(GetActorLocation());
}

void ANeutralCamp::VisualDeathMulticast_Implementation(FOwner Depleter)
{
	VisualDeathBP(Depleter);
}

void ANeutralCamp::VisualRespawnTimeMulticast_Implementation(float TimeRemaining, float TotalTime)
{
	if (HasAuthority()) return;
	VisualRespawnTimeBP(TimeRemaining, TotalTime);
}

void ANeutralCamp::OnAttackPerformedMulticast_Implementation(const FAttackHitInfo& AttackHitInfo)
{
	OnAttackPerformedBP(AttackHitInfo);
}

void ANeutralCamp::OnAttackPerformed(FAttackHitInfo AttackHitInfo)
{
	OnAttackPerformedMulticast(AttackHitInfo);
}

void ANeutralCamp::RotateStaticMeshesAroundPoint(TArray<UStaticMeshComponent*> StaticMeshes, FVector Location, float AngleRotationSpeed, float DeltaTime)
{
	if (StaticMeshes.Num() == 0) return;
	for (auto StaticMesh : StaticMeshes)
	{
		if (!StaticMesh) continue;
		FVector Direction = StaticMesh->GetComponentLocation() - Location;
		FRotator Rotation = FRotator(0, AngleRotationSpeed * DeltaTime, 0);
		Direction = Rotation.RotateVector(Direction);
		FVector NewLocation = Location + Direction;
		StaticMesh->SetWorldLocation(NewLocation);
		FRotator NewRotation = FRotator(0, AngleRotationSpeed * DeltaTime, 0);
		StaticMesh->AddLocalRotation(NewRotation);
	}
}
