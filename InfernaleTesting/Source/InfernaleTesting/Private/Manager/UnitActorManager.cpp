// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/UnitActorManager.h"

#include "Component/ActorComponents/AttackComponent.h"
#include "Component/ActorComponents/BattleManagerComponent.h"
#include "Component/ActorComponents/PathfindingGridComponent.h"
#include "Component/ActorComponents/UnitActorGridComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "DataAsset/UnitActorDataAsset.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Interfaces/Damageable.h"
#include "LD/Buildings/Building.h"
#include "LD/Buildings/BuildingParent.h"
#include "LD/Buildings/MainBuilding.h"
#include "LD/LDElement/LDElement.h"
#include "Mass/Army/AmalgamFragments.h"
#include "UnitAsActor/UnitActor.h"

// Sets default values
AUnitActorManager::AUnitActorManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	GridComponent = CreateDefaultSubobject<UUnitActorGridComponent>(TEXT("GridComponent"));
	//PathfindingGridComponent = CreateDefaultSubobject<UPathfindingGridComponent>(TEXT("PathfindingGridComponent"));
	//BattleManagerComponent = CreateDefaultSubobject<UBattleManagerComponent>(TEXT("BattleManagerComponent"));
}

// Called every frame
void AUnitActorManager::Tick(float DeltaTime)
{
	BlueprintEditorTick(DeltaTime);
	Super::Tick(DeltaTime);
	
	if (UnitActorsToSync.Num() > 0)
	{
		auto UnitActor = UnitActorsToSync[0];
		if (!UnitActor.IsValid()) return;
		UnitActor->RefreshFluxPathOwnFlux();
		UnitActorsToSync.RemoveAt(0);
	}
}

bool AUnitActorManager::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AUnitActorManager::BlueprintEditorTick(float DeltaTime)
{
	if (!GetWorld()) return;
	//PathfindingGridComponent->BlueprintEditorTick(DeltaTime);
}

void AUnitActorManager::SpawnUnit(TWeakObjectPtr<ABuildingParent> Building)
{
	auto Fluxes = Building->GetFluxes();
	auto BuildingOwner = Building->GetOwner();
	auto UnitStruct = UnitStructs[0];
	
	auto SpawnClass = UnitStruct.UnitActorClass->GetDefaultObject()->GetClass();
	FTransform Transform = FTransform();
	Transform.SetLocation(FVector(Building->GetActorLocation()));
	Transform.SetRotation(Building->GetActorRotation().Quaternion());
	Transform.SetScale3D(FVector(1, 1, 1));
	FActorSpawnParameters SpawnParams;

	for (auto Flux : Fluxes)
	{
		AActor* Actor = GetWorld()->SpawnActor(SpawnClass, &Transform, SpawnParams);
		auto UnitActor = Cast<AUnitActor>(Actor); 
		if (!UnitActor)
		{
			Actor->Destroy();
			continue;
		}
	
		UnitActor->Init(UnitStruct, BuildingOwner, Flux, TWeakObjectPtr<AUnitActorManager>(this));
		GridComponent->AddUnitActor(TWeakObjectPtr<AUnitActor>(UnitActor));
		UnitActor->UnitStateAsk.AddDynamic(this, &AUnitActorManager::OnUnitStateAsk);
	}
}

TWeakObjectPtr<UUnitActorGridComponent> AUnitActorManager::GetGridComponent() const
{
	return GridComponent;
}

bool AUnitActorManager::Attack(TWeakObjectPtr<AActor> Attacker, FAttackStruct AttackStruct, EAttackerType AttackerType)
{
	/* Used by MainBuilding and Neutral Camp */
	auto Center = Attacker->GetActorLocation();
	auto ForwardVector = Attacker->GetActorForwardVector();
	FUnitActorGridCell Cell = GetAllInRange(Center, AttackStruct.AttackRange, AttackStruct.AttackAngle, ForwardVector, false);
	auto Ownable = Cast<IOwnable>(Attacker);
	if (!Ownable)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Attacker is not Ownable in AUnitActorManager::Attack"));
		return false;
	}
	FOwner AttackerOwner = Ownable->GetOwner();
	
	TWeakObjectPtr<AActor> ClosestActor = nullptr;
	auto ClosestDistance = TNumericLimits<float>::Max();
	EUnitTargetType TargetType = EUnitTargetType::UTargetNone;
	
	// Check for Enemies
	if (AttackStruct.AttackUnits)
	{
		for (auto Unit : Cell.UnitActors)
		{
			if (!Unit.IsValid()) continue;
			if (Unit->GetOwner().Team != AttackerOwner.Team)
			{
				const auto Distance = FVector::Dist(Center, Unit->GetActorLocation());
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ClosestActor = Unit;
					TargetType = EUnitTargetType::UTargetUnit;
				}
				continue;
			} 
		}
	}

	if (AttackStruct.AttackBuildings)
	{
		for (auto Building : Cell.Buildings)
		{
			if (!Building.IsValid()) continue;
			if (Building->GetOwner().Team != AttackerOwner.Team)
			{
				const auto Distance = FVector::Dist(Center, Building->GetActorLocation());
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ClosestActor = Building;
					TargetType = EUnitTargetType::UTargetBuilding;
				}
				continue;
			}
		}
	}
	
	/* Check for Neutral Camps */
	if (AttackStruct.AttackNeutral)
	{
		for (auto LDElement : Cell.LDElements)
		{
			if (!LDElement.IsValid()) continue;
			const auto Distance = FVector::Dist(Center, LDElement->GetActorLocation());
	
			// Remove later
			if (LDElement->GetLDElementType() != ELDElementType::LDElementNeutralCampType) continue;
			
			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestActor = LDElement;
				TargetType = EUnitTargetType::UTargetNeutralCamp;
			}
		}
	}
	
	if (ClosestActor == nullptr)
	{
		return false;
	}
	
	const auto DebugStart = Center + FVector(0, 0, 100);

	if (AttackerType == EAttackerType::AttackerTypeBuilding) DrawDebugLineMulticast(DebugStart, ClosestActor->GetActorLocation(), FColor::Purple, .3f, 100);

	auto Distance = FVector::Dist(Center, ClosestActor->GetActorLocation());

	
	const auto Targetable = Cast<IUnitTargetable>(ClosestActor);
	const auto AttackAimRange = AttackStruct.AttackRange + Targetable->GetTargetableRange();
	const bool UnitInRange = Distance <= AttackAimRange;
	if (!UnitInRange)
	{
		return false;
	}

	auto Offset = (AttackerType == EAttackerType::AttackerTypeBuilding) ? FVector(0, 0, 1350.f) : FVector(0, 0, 0.f);
	//DrawDebugLine(GetWorld(), DebugStart+Offset, ClosestActor->GetActorLocation(), FColor::Purple, false, .3f, 0, 100);
	
	const auto Damageable = Cast<IDamageable>(ClosestActor);
	Damageable->DamageHealthOwner(AttackStruct.AttackDamage, false, AttackerOwner);
	return true;
}

FTransmutationEffects AUnitActorManager::GetEffect(const ENodeEffect NodeEffect, FOwner EffectOwner) const
{
	return TransmutationComponents[EffectOwner.Player]->GetEffect(NodeEffect);
}

float AUnitActorManager::ApplyEffect(float InitialValue, const ENodeEffect NodeEffect, FOwner EffectOwner) const
{
	return TransmutationComponents[EffectOwner.Player]->ApplyEffect(InitialValue, NodeEffect);
}

void AUnitActorManager::AddUnitToPathfind(TWeakObjectPtr<AUnitActor> UnitActor)
{
	UnitActorsToSync.Add(UnitActor);
}

TMap<EPlayerOwning, FUnitActors>& AUnitActorManager::GetUnitActorsByPlayers()
{
	return UnitActorsByPlayers;
}

TWeakObjectPtr<UTransmutationComponent> AUnitActorManager::GetTransmutationComponent(EPlayerOwning Player) const
{
	for (auto TransmutationComponentLocal : TransmutationComponents)
	{
		if (TransmutationComponentLocal.Key == Player) return TransmutationComponentLocal.Value;
	}
	return nullptr;
}

TWeakObjectPtr<UBattleManagerComponent> AUnitActorManager::GetBattleManagerComponent() const
{
	return BattleManagerComponent;
}

TArray<IFluxRepulsor*> AUnitActorManager::GetRepulsorsInRange(FVector Location, float Range)
{
	return GridComponent->GetRepulsorsInRange(Location, Range);
}

TArray<AMainBuilding*> AUnitActorManager::GetMainBuildings() const
{
	return MainBuildings;
}

void AUnitActorManager::DrawDebugLineMulticast_Implementation(FVector Start, FVector End, FColor Color, float LifeTime,
                                                              float Thickness)
{
	DrawDebugLine(GetWorld(), Start, End, Color, false, LifeTime, 0, Thickness);
}

// Called when the game starts or when spawned
void AUnitActorManager::BeginPlay()
{
	Super::BeginPlay();
	SyncWithDataAsset();
	
	if (!HasAuthority()) return;
	const auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	GameMode->PreLaunchGame.AddDynamic(this, &AUnitActorManager::OnPreLaunchGame);
	GameMode->LaunchGame.AddDynamic(this, &AUnitActorManager::OnLaunchGame);
}

void AUnitActorManager::SyncWithDataAsset()
{
	if (!bUseDataAsset) return;
	if (!UnitActorDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("UnitActorDataAsset not found in AUnitActorManager::SyncWithDataAsset"));
		return;
	}

	UnitStructs = UnitActorDataAsset->UnitStructs;
}

void AUnitActorManager::CheckUnitRange(TWeakObjectPtr<AUnitActor> UnitActor)
{
	GridComponent->CheckUnitRange(UnitActor);
}

void AUnitActorManager::CheckUnitAttack(TWeakObjectPtr<AUnitActor> UnitActor)
{
	if (!UnitActor.IsValid()) return;
	
	const auto Target = UnitActor->GetTarget();
	if (!Target.IsValid()) return;
	const auto TargetUnitAsDamageable = Cast<IDamageable>(Target);
	const auto TargetAsOwnable = Cast<IOwnable>(Target);
	if (TargetAsOwnable->GetOwner().Team == UnitActor->GetOwner().Team)
	{
		UnitActor->SetTarget(nullptr, EUnitTargetType::UTargetNone);
		return;
	}
	//Check range
	const auto Distance = FVector::Dist(UnitActor->GetActorLocation(), Target->GetActorLocation());
	const auto Targetable = Cast<IUnitTargetable>(Target);
	const auto AttackAimRange = UnitActor->GetUnitStruct().BaseRange + Targetable->GetTargetableRange();
	const bool UnitInRange = Distance <= AttackAimRange;
	//GEngine->AddOnScreenDebugMessage(-1, 50.f, UnitInRange ? FColor::Green : FColor::Red, FString::Printf(TEXT("Attacker targetted at %f (Max: %f"), Distance, UnitRange));
	if (!UnitInRange) return;
	
	auto Damage = UnitActor->GetUnitStruct().BaseDamage;
	switch (UnitActor->GetTargetType()) {
	case EUnitTargetType::UTargetNone:
		break;
	case EUnitTargetType::UTargetBuilding:
		Damage = TransmutationComponents[UnitActor->GetOwner().Player]->GetEffectDamageToBuilding(Damage);
		break;
	case EUnitTargetType::UTargetUnit:
		Damage = TransmutationComponents[UnitActor->GetOwner().Player]->GetEffectDamageToUnit(Damage);
		break;
	case EUnitTargetType::UTargetNeutralCamp:
		Damage = TransmutationComponents[UnitActor->GetOwner().Player]->GetEffectDamageToMonster(Damage);
		break;
	}
	FBattleInfoCode BattleInfo;
	BattleInfo.AttackerUnitType = EEntityType::EntityTypeBehemot;
	BattleInfo.TargetUnitType = EEntityType::EntityTypeBehemot;
	const auto UnitLoc = UnitActor->GetActorLocation();
	const auto TargetLoc = Target->GetActorLocation();
	BattleInfo.BattlePositionAttackerWorld = FVector2D(UnitLoc.X, UnitLoc.Y);
	BattleInfo.BattlePositionTargetWorld = FVector2D(TargetLoc.X, TargetLoc.Y);
	BattleInfo.AttackerOwner = UnitActor->GetOwner();
	BattleInfo.TargetOwner = TargetAsOwnable->GetOwner();
	BattleInfo.UnitTargetTypeTarget = UnitActor->GetTargetType();

	
	TargetUnitAsDamageable->DamageHealthOwner(Damage, false, UnitActor->GetOwner());
	BattleManagerComponent->AtPosBattleInfo(BattleInfo);
	//DrawDebugLineMulticast(UnitActor->GetActorLocation(), Target->GetActorLocation(), FColor::Red, .3f, 100);
}

FUnitActorGridCell AUnitActorManager::GetAllInRange(FVector Location, float Range, float Angle, FVector Forward, bool bDebug)
{
	if (!GridComponent) return FUnitActorGridCell();
	if (!GridComponent->WasGridCreated()) return FUnitActorGridCell();
	return GridComponent->GetAllInRange(Location, Range, Angle, Forward, bDebug);
}

void AUnitActorManager::HandleEffectChanged(EPlayerOwning Player, ENodeEffect Effect, UTransmutationComponent* TransmutationComponent)
{
	switch (Effect)
	{
	case (ENodeEffect::NodeEffectHealthUnit):
		UpdateHealthUnit(Player, TransmutationComponent);
		break;
	case (ENodeEffect::NodeEffectHealthBuilding):
		UpdateHealthBuilding(Player, TransmutationComponent);
		break;
	case (ENodeEffect::NodeEffectUnitSight):
		UpdateSightUnit(Player, TransmutationComponent);
		break;
	case (ENodeEffect::NodeEffectBuildingSight):
		UpdateSightBuilding(Player, TransmutationComponent);
		break;
	case (ENodeEffect::NodeEffectBuildingConstructionTime):
		UpdateConstructionTimeBuilding(Player, TransmutationComponent);
		break;
	case (ENodeEffect::NodeEffectBuildingOverclockDuration):
		UpdateOverclockDurationBuilding(Player, TransmutationComponent);
		break;
	// case (ENodeEffect::NodeEffectUnitSight):
	// 	UpdateSightUnit(Player);
	default:
		return;
	}
}

void AUnitActorManager::UpdateHealthUnit(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent)
{
	TArray<TWeakObjectPtr<AUnitActor>> Units = UnitActorsByPlayers[Player].UnitActors;
	for (const auto Unit : Units)
	{
		if (!Unit.IsValid()) continue;
		const auto NewMaxHealth = TransmutationComponent->ApplyEffect(Unit->GetUnitStruct().BaseHealth, ENodeEffect::NodeEffectHealthUnit);
		Unit->UpdateMaxHealth(NewMaxHealth);
	}
}

void AUnitActorManager::UpdateHealthBuilding(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent)
{
	TArray<TWeakObjectPtr<ABuildingParent>> BuildingParents = TArray<TWeakObjectPtr<ABuildingParent>>();
	GridComponent->GetBuildingsOfPlayer(BuildingParents, Player);

	for (const auto BuildingParent : BuildingParents)
	{
		if (!BuildingParent.IsValid()) continue;
		if (BuildingParent->IsMainBuilding())
		{
			const auto MainBuilding = Cast<AMainBuilding>(BuildingParent);
			MainBuilding->AskRefreshMaxHealth();
			continue;
		}
		const auto Building = Cast<ABuilding>(BuildingParent);
		const auto NewMaxHealth = TransmutationComponent->ApplyEffect(Building->GetBuildingInfo().MaxHealth, ENodeEffect::NodeEffectHealthBuilding);
		Building->UpdateMaxHealth(NewMaxHealth);
	}
}

void AUnitActorManager::UpdateSightUnit(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent)
{
	TArray<TWeakObjectPtr<AUnitActor>> Units = UnitActorsByPlayers[Player].UnitActors;
	for (const auto Unit : Units)
	{
		if (!Unit.IsValid()) continue;
		const auto NewSight = TransmutationComponent->ApplyEffect(Unit->GetUnitStruct().BaseSightRange, ENodeEffect::NodeEffectUnitSight);
		Unit->UpdateSightFogOfWar(NewSight);
	}
}

void AUnitActorManager::UpdateSightBuilding(EPlayerOwning Player, UTransmutationComponent* TransmutationComponent)
{
	TArray<TWeakObjectPtr<ABuildingParent>> BuildingParents = TArray<TWeakObjectPtr<ABuildingParent>>();
	GridComponent->GetBuildingsOfPlayer(BuildingParents, Player);

	for (const auto BuildingParent : BuildingParents)
	{
		//TODO update sight of building for fog of war
	}
}

void AUnitActorManager::UpdateConstructionTimeBuilding(EPlayerOwning Player,
	UTransmutationComponent* TransmutationComponent)
{
	TArray<TWeakObjectPtr<ABuildingParent>> BuildingParents = TArray<TWeakObjectPtr<ABuildingParent>>();
	GridComponent->GetBuildingsOfPlayer(BuildingParents, Player);

	for (const auto BuildingParent : BuildingParents)
	{
		if (!BuildingParent.IsValid()) continue;
		if (BuildingParent->IsMainBuilding()) continue;
		
		const auto Building = Cast<ABuilding>(BuildingParent);
		Building->UpdateConstructionTime();
	}
}

void AUnitActorManager::UpdateOverclockDurationBuilding(EPlayerOwning Player,
	UTransmutationComponent* TransmutationComponent)
{
	TArray<TWeakObjectPtr<ABuildingParent>> BuildingParents = TArray<TWeakObjectPtr<ABuildingParent>>();
	GridComponent->GetBuildingsOfPlayer(BuildingParents, Player);

	for (const auto BuildingParent : BuildingParents)
	{
		if (!BuildingParent.IsValid()) continue;
		if (BuildingParent->IsMainBuilding()) continue;
		
		const auto Building = Cast<ABuilding>(BuildingParent);
		Building->UpdateOverclockDuration();
	}
}

void AUnitActorManager::OnUnitStateAsk(AUnitActor* UnitActor, const EUnitStateAsk UnitStateAsk)
{
	const TWeakObjectPtr<AUnitActor> UnitActorWeak = TWeakObjectPtr<AUnitActor>(UnitActor);
	switch (UnitStateAsk)
	{
		case UnitStateAskNone:
			break;
		case EUnitStateAsk::UnitStateAskRange:
			CheckUnitRange(UnitActorWeak);
			break;
		case UnitStateAskAttack:
			CheckUnitAttack(UnitActorWeak);
			break;
	}
}

void AUnitActorManager::OnPreLaunchGame()
{
	const auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	const auto PlayerStates = GameMode->GetPlayerStates();
	for (auto PlayerState : PlayerStates)
	{
		const auto PC = Cast<APlayerControllerInfernale>(PlayerState->GetPlayerController());
		if (!PC) continue;
		const auto TransmutationComponent = PC->GetTransmutationComponent();
		TransmutationComponents.Add(PlayerState->GetOwnerInfo().Player, TransmutationComponent);
		UnitActorsByPlayers.Add(PlayerState->GetOwnerInfo().Player, FUnitActors());
		TransmutationComponent->NodeOwnedOwnerShipAltered.AddDynamic(this, &AUnitActorManager::OnNodeOwnedOwnerShipAltered);
		TransmutationComponent->SimpleNodeOwnedOwnerShipAltered.AddDynamic(this, &AUnitActorManager::OnNodeOwnedOwnerShipAltered);
	}
}

void AUnitActorManager::OnLaunchGame()
{
	BattleManagerComponent->OnLaunchGame();
}

void AUnitActorManager::OnNodeOwnedOwnerShipAltered(TArray<ENodeEffect> Effects, UTransmutationComponent* TransmutationComponent)
{
	for (auto TransmutationComponentInfo : TransmutationComponents)
	{
		auto LocalTransmutationComponent = TransmutationComponentInfo.Value;
		if (LocalTransmutationComponent != TransmutationComponent) continue;

		for (auto Effect : Effects)
		{
			HandleEffectChanged(TransmutationComponentInfo.Key, Effect, LocalTransmutationComponent);
		}
		return;
	}
}

