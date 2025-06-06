// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/Buildings/Building.h"

#include "Component/ActorComponents/AttackComponent.h"
#include "Component/ActorComponents/BuildingEffectComponent.h"
#include "Component/ActorComponents/ConstructWithDelayComponent.h"
#include "Component/ActorComponents/DamageableComponent.h"
#include "Component/ActorComponents/DestroyedAfterDurationComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "Component/ActorComponents/SpawnerComponent.h"
#include "Component/ActorComponents/UnitActorGridComponent.h"
#include "Component/PlayerController/UIComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "Components/WidgetComponent.h"
#include "DataAsset/AttacksDataAsset.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "Flux/Flux.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "LD/Buildings/MainBuilding.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Breach.h"
#include "Manager/UnitActorManager.h"
#include "Mass/Collision/SpatialHashGrid.h"
#include "Component/ActorComponents/BattleManagerComponent.h"

ABuilding::ABuilding()
{
	// Create root component
	DestroyedAfterDurationComponent = CreateDefaultSubobject<UDestroyedAfterDurationComponent>(TEXT("DestroyedAfterDurationComponent"));
	ConstructWithDelayComponent = CreateDefaultSubobject<UConstructWithDelayComponent>(TEXT("ConstructWithDelayComponent"));
	ConstructionWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ConstructionWidgetComponent"));
	SpawnerComponent = CreateDefaultSubobject<USpawnerComponent>(TEXT("SpawnerComponent"));
	BuildingEffectComponent = CreateDefaultSubobject<UBuildingEffectComponent>(TEXT("BuildingEffectComponent"));
	
	ConstructionWidgetComponent->SetupAttachment(RootComponent);
	ConstructWithDelayComponent->LinkToDamageableComponent(TWeakObjectPtr<UDamageableComponent>(DamageableComponent));
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();
	DestroyedAfterDurationComponent->BuildingDestroyed.AddDynamic(this, &ABuilding::OnBuildingDestroyedByTime);
	ConstructWithDelayComponent->ConstructionFinished.AddDynamic(this, &ABuilding::OnConstructionCompleted);
	ConstructWithDelayComponent->ConstructionTimeRemaining.AddDynamic(this, &ABuilding::OnConstructionTimeRemaining);
	//SetReplicates(true);

	bUseMass = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseMass;

	if (!HasAuthority()) return;
	if (AttackComponent)
	{
		AttackComponent->AttackReadyDelegate.AddDynamic(this, &ABuilding::OnAttackReady);

		FTimerHandle TimerHandle_InitAttacks;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_InitAttacks, this, &ABuilding::InitAttacks, 0.1f, false);
	}
}

void ABuilding::DestroyBuilding(const bool bDestroyedByUnit)
{
	// Destroy effects;
	if (HasAuthority())
	{
		//ASpatialHashGrid::RemoveBuildingFromCell(GetActorLocation(), this);
		//ASpatialHashGrid::RemoveBuildingFromGrid(GetActorLocation(), this);
		//UnitActorManager->GetGridComponent()->RemoveBuilding(TWeakObjectPtr<ABuildingParent>(this));
		RemoveBuildingFromOwner();
		// for (const auto Flux : Fluxes)
		// {
		// 	Flux->RemoveFluxServer();
		// }
	}
	if (bIsHovered)
	{
		APlayerControllerInfernale* PlayerController = Cast<APlayerControllerInfernale>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (PlayerController) PlayerController->GetUIComponent()->SoulsCostHovered(false, 0);
	}
	
	BuildingDestroyed.Broadcast(this);
	BuildingDestroyedMulticast();
	BuildingParentDestroyed.Broadcast(this);
	Destroy();
}

void ABuilding::SetBuildingInfo(const FBuildingStruct& NewBuildingInfo, UTransmutationComponent* NewTransmutationComponent)
{
	BuildingInfo = NewBuildingInfo;
	bIsBuildingInfoSet = true;
	BuildingStructUpdated.Broadcast(this);
	ReplicateBuildingInfoMulticast(BuildingInfo);
	LocalTransmutationComponent = NewTransmutationComponent;
	SetLifeTime(false);
	CalculAndUpdateConstructionTime(false);

	auto MaxHealth = NewTransmutationComponent->ApplyEffect(NewBuildingInfo.MaxHealth, ENodeEffect::NodeEffectHealthBuilding);
	SetBuildingHealth(MaxHealth);
}

void ABuilding::SetBreach(ABreach* Parent)
{
	Breach = Parent;
	Breach->GetMainBuilding()->BuildingParentFluxesUpdated.AddDynamic(this, &ABuilding::BroadcastBuildingParentFluxesUpdated);
	Breach->BreachOwnershipChanged.AddDynamic(this, &ABuilding::OnBreachOwnershipChanged);

	//ASpatialHashGrid::AddBuildingToGrid(GetActorLocation(), this);
}

void ABuilding::ConstructionTimeRemaining_Implementation(const float RemainingTime, const float InitialDuration, const bool shouldHide)
{
	OnConstructionTimeRemainingBP(RemainingTime, InitialDuration, shouldHide);
}

void ABuilding::SetPermanentMulticast_Implementation(const bool IsPermanent)
{
	bIsPermanent = IsPermanent;
	if (bIsPermanent) bShouldBinkForLifeTime = false;
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Building is permanent"));
	UpgradeBuildingVisuals();

	if (bIsHovered) InteractedBP(false);

	if (IsPermanent) BuildingMadePermanent.Broadcast(this);
	
	if (DestroyedAfterDurationComponent == nullptr) return;
	DestroyedAfterDurationComponent->StopDestroying();
}

void ABuilding::SetPermanent(const bool IsPermanent)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PlayerController) return;

	APlayerControllerInfernale* PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
	if (!PlayerControllerInfernale) return;

	PlayerControllerInfernale->SetBuildingPermanent(this, IsPermanent);
}

void ABuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bShouldBinkForLifeTime) return;

	BlinkingTime += DeltaTime;
	if (BlinkingTime < BlinkDelay) return;

	BlinkEffect();
	BlinkingTime = 0;
	
}

void ABuilding::OnBuildingConstructedMulticast_Implementation(const bool bConstructed)
{
	bIsConstructed = bConstructed;
	//if (!IsPermanent()) bShouldBinkForLifeTime = true;
	if (!bIsHovered) return;

	InteractedBP(true);
}

void ABuilding::BuildingDestroyedMulticast_Implementation()
{
	if (HasAuthority())
	{
		RemoveAllFluxes();
		return;
	}
	BuildingDestroyed.Broadcast(this);
	BuildingParentDestroyed.Broadcast(this);
}

void ABuilding::ReplicateBuildingInfoMulticast_Implementation(FBuildingStruct NewBuildingInfo)
{
	if (HasAuthority()) return;
	BuildingInfo = NewBuildingInfo;
	bIsBuildingInfoSet = true;
	BuildingStructUpdated.Broadcast(this);
}

void ABuilding::UpgradeBuildingVisuals_Implementation()
{
	// Upgrade visuals
}

void ABuilding::SetOverclockedMulticast_Implementation(const bool IsOverclocked)
{
	bOverclocked = IsOverclocked;
	bShouldBinkForLifeTime = IsOverclocked;
	if (IsOverclocked) BuildingOverclocked.Broadcast(this);
	else BuildingOverclockEnded.Broadcast(this);
	OverclickVisualsBP();
}

void ABuilding::SetLifeTime(bool KeepProgress) const
{
	if (DestroyedAfterDurationComponent == nullptr) return;

	if (!LocalTransmutationComponent.IsValid())
	{
		if (KeepProgress) DestroyedAfterDurationComponent->SetDurationKeepProgress(20, 1);
		else DestroyedAfterDurationComponent->SetDuration(20, 1);
		return;
	}
	
	auto Value = BuildingInfo.LifeTime;
	auto Effect = LocalTransmutationComponent->GetEffect(ENodeEffect::NodeEffectBuildingOverclockDuration);
	Value += Effect.ValueFlat + Effect.ValueFlatCurse;
	auto Modifier = (Effect.ValuePercent * Effect.ValuePercentCurse);
	
	if (KeepProgress) DestroyedAfterDurationComponent->SetDurationKeepProgress(Value, Modifier);
	else DestroyedAfterDurationComponent->SetDuration(Value, Modifier);
}

void ABuilding::SetConstructionTime(const float Duration, const float SpeedModifier, bool KeepProgress) const
{
	if (ConstructWithDelayComponent == nullptr) return;
	if (KeepProgress) ConstructWithDelayComponent->SetConstructionTimeKeepProgress(Duration, SpeedModifier);
	else ConstructWithDelayComponent->SetConstructionTime(Duration, SpeedModifier);
	ConstructWithDelayComponent->StartConstruction();
}

void ABuilding::SetBuildingHealth(const float MaxHealth) const
{
	if (DamageableComponent == nullptr) return;
	DamageableComponent->SetMaxHealth(MaxHealth, true, true, false, 1);
}

void ABuilding::RemoveBuildingFromOwner()
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PlayerController) return;

	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerController->PlayerState);
	if (!PlayerState) return;

	PlayerState->GetEconomyComponent()->RemoveBuilding(this);
}

void ABuilding::AddBuildingToOwner()
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PlayerController) return;

	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerController->PlayerState);
	if (!PlayerState) return;

	PlayerState->GetEconomyComponent()->AddBuilding(this);
}

void ABuilding::CalculAndUpdateConstructionTime(bool KeepProgress)
{
	if (LocalTransmutationComponent == nullptr)
	{
		SetConstructionTime(10, 1, KeepProgress);
		return;
	}
	auto BuildTime = BuildingInfo.ConstructionTime;
	auto Effect = LocalTransmutationComponent->GetEffect(ENodeEffect::NodeEffectBuildingConstructionTime);
	BuildTime += Effect.ValueFlat + Effect.ValueFlatCurse;
	auto BuildSpeed = Effect.ValuePercent * Effect.ValuePercentCurse;
	SetConstructionTime(BuildTime, BuildSpeed, KeepProgress);
}

AMainBuilding* ABuilding::GetMainBuilding()
{
	if (!Breach) return nullptr;
	return Breach->GetMainBuilding();
}

void ABuilding::OnSelectedForFluxCpp()
{
	if (!Breach) return;
	Breach->GetMainBuilding()->OnSelectedForFluxCpp();
}

void ABuilding::OnDeselectedForFluxCpp()
{
	if (!Breach) return;
	Breach->GetMainBuilding()->OnDeselectedForFluxCpp();
}

float ABuilding::GetRepulsorRange() const
{
	return BuildingInfo.RepulsorRange;
}

UDestroyedAfterDurationComponent* ABuilding::GetDestroyedAfterDurationComponent() const
{
	return DestroyedAfterDurationComponent;
}

void ABuilding::SetPermanentFromServer(const bool IsPermanent)
{
	if (!HasAuthority()) return;
	SetPermanentMulticast(IsPermanent);
}

bool ABuilding::IsFullyConstructed() const
{
	return bIsConstructed;
}

bool ABuilding::IsPermanent() const
{
	return bIsPermanent;
}

void ABuilding::SetOverclocked()
{
	if (!HasAuthority()) return;
	SetLifeTime(false);
	SetOverclockedMulticast(true);
	DestroyedAfterDurationComponent->StartDestroying();
	//DestroyedAfterDurationComponent->LifeTimeReduced.AddDynamic(this, &ABuilding::LifeTimeRemainingMulticast);
}

bool ABuilding::IsOverclocked()
{
	return bOverclocked;
}

void ABuilding::UpdateOverclockDuration()
{
	if (!HasAuthority()) return;
	if (!bOverclocked) return;
	SetLifeTime(true);
}

void ABuilding::UpdateConstructionTime()
{
	if (!HasAuthority()) return;
	if (bIsConstructed) return;
	CalculAndUpdateConstructionTime(true);
}

void ABuilding::SetAttackOverclocked(const bool bOverclockedVal)
{
	if (!HasAuthority()) return;
	if (AttackComponent == nullptr) return;
	AttackComponent->SetAttacks(bOverclockedVal ? AttacksOverclockDataAsset->Attacks : AttacksDataAsset->Attacks);
}

float ABuilding::GetConstructionTime() const
{
	if (ConstructWithDelayComponent == nullptr) return 0;
	return ConstructWithDelayComponent->GetConstructionTimeRemaining();
}

void ABuilding::InteractStartHover(APlayerControllerInfernale* Interactor)
{
	if (Interactor->bIsEscapeMenuOpen) return;
	Super::InteractStartHover(Interactor);
	if (Interactor->GetTeam() != OwnerWithTeam.Team) return;
	
	if (bIsConstructed) HoveredBP(true); // Should depend on if the player can afford the upgrade
}

void ABuilding::InteractEndHover(APlayerControllerInfernale* Interactor)
{
	Super::InteractEndHover(Interactor);
	bIsHovered = false;
	HoveredBP(false);
}

void ABuilding::InteractStartMain(APlayerControllerInfernale* Interactor)
{
	Super::InteractStartMain(Interactor);
	if (Interactor->GetTeam() != OwnerWithTeam.Team) return;
	
	bIsHovered = true;
	LastInteractor = Interactor;
	Interactor->MouseSecondaryEnd.AddDynamic(this, &ABuilding::EndInteraction);
	Interactor->EscapeStarted.AddDynamic(this, &ABuilding::EndInteraction);
	Interactor->MousePrimaryStart.AddDynamic(this, &ABuilding::InteractionMousePrimary);

	
	if (bIsConstructed) InteractedBP(true);
}



bool ABuilding::CanCreateAFlux() const
{
	return bCanCreateFlux;
}

TArray<TWeakObjectPtr<AFlux>> ABuilding::GetFluxes()
{
	const auto MainBuilding = GetMainBuilding();
	if (!MainBuilding) return TArray<TWeakObjectPtr<AFlux>>();
	return MainBuilding->GetFluxes();
}

TArray<TWeakObjectPtr<AFlux>> ABuilding::GetLocalFluxes()
{
	return Fluxes;
}

void ABuilding::ChangeOwner(FOwner NewOwner)
{
	Super::ChangeOwner(NewOwner);
	AddBuildingToOwner();
}

FBuildingStruct ABuilding::GetBuildingInfo() const
{
	return BuildingInfo;
}

UBattleManagerComponent* ABuilding::GetBattleManager()
{
	return GetMainBuilding()->GetBattleManager();
}

void ABuilding::EndInteraction()
{
	if (LastInteractor)
	{
		LastInteractor->MouseSecondaryEnd.RemoveDynamic(this, &ABuilding::EndInteraction);
		LastInteractor->EscapeStarted.RemoveDynamic(this, &ABuilding::EndInteraction);
		LastInteractor->MousePrimaryStart.RemoveDynamic(this, &ABuilding::InteractionMousePrimary);
		LastInteractor->CallInteractionDone();
	}
	bIsHovered = false;
	InteractedBP(false);
}

void ABuilding::InteractionMousePrimary()
{
	FHitResult HitResult;
	if (!LastInteractor) return;
	LastInteractor->GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery1, true, HitResult);
	if (HitResult.GetActor() == this) return;
	EndInteraction();
}

void ABuilding::AddFlux(AFlux* Flux)
{
	Fluxes.Add(Flux);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Flux Added"));
}

void ABuilding::RemoveAllFluxes()
{
	for (const auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->RemoveFluxServer();
	}
	Fluxes.Empty();
	GetMainBuilding()->AskUpdateSpawners();
}

USpawnerComponent* ABuilding::GetSpawnerComponent()
{
	return SpawnerComponent;
}

bool ABuilding::WasBuildingInfoSet() const
{
	return bIsBuildingInfoSet;
}

void ABuilding::OnBuildingDestroyedByTime(ABuilding* Building)
{
	if (!HasAuthority()) return;
	SetOverclockedMulticast(false);
}

void ABuilding::OnHealthDepleted()
{
	Super::OnHealthDepleted();
	if (bDebugHealth) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Building Health Depleted"));
	if (!HasAuthority()) return;
	if (bDebugHealth) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Building Health Depleted Server"));
	DestroyBuilding(true);
}

void ABuilding::OnHealthDepletedOwner(AActor* Actor, FOwner Depleter)
{
	Super::OnHealthDepletedOwner(Actor, Depleter);
	OnHealthDepleted();
}

void ABuilding::OnConstructionCompleted(AActor* Building)
{
	if (!HasAuthority()) return;
	ConstructionTimeRemaining(0, 1, true);
	SetPermanent(true);
	BuildingConstructed.Broadcast(this);
	OnBuildingConstructedMulticast(true);
}


void ABuilding::OnConstructionTimeRemaining(float RemainingTime, float InitialDuration)
{
	if (!HasAuthority()) return;
	ConstructionTimeRemaining(RemainingTime, InitialDuration, false);
}

void ABuilding::OnBreachOwnershipChanged(ABreach* BreachChanged, FOwner NewOwner, FOwner OldOwner)
{
	if (!HasAuthority()) return;
	DestroyBuilding();
}

void ABuilding::OnAttackReady(FAttackStruct AttackStruct)
{
	if (bUseMass)
	{
		auto Closest = ASpatialHashGrid::FindClosestEntity(GetActorLocation(), AttackStruct.AttackRange, AttackStruct.AttackAngle, GetActorForwardVector(), FMassEntityHandle(0, 0), GetOwner().Team);
		if (!Closest.IsSet()) return;
	
		ASpatialHashGrid::DamageEntity(Closest, AttackStruct.AttackDamage);

		GridCellEntityData TargetData = ASpatialHashGrid::GetEntityData(Closest);

		UBattleManagerComponent* BattleManager = GetBattleManager();

		if (BattleManager)
		{
			FBattleInfoCode BattleInfo = FBattleInfoCode();
			FVector AttackLocation = GetActorLocation();
			BattleInfo.AttackerUnitType = EEntityType::EntityTypeBuilding;
			BattleInfo.TargetUnitType = TargetData.EntityType;
			BattleInfo.BattlePositionAttackerWorld = FVector2D(AttackLocation.X, AttackLocation.Y);
			FVector TgtLocation = TargetData.Location;
			BattleInfo.BattlePositionTargetWorld = FVector2D(TgtLocation.X, TgtLocation.Y); 
			BattleInfo.AttackerOwner = OwnerWithTeam;
			BattleInfo.TargetOwner = TargetData.Owner;
			BattleInfo.TargetID = Closest;
			BattleInfo.AttackerID = Closest;
			BattleManager->AtPosBattleInfo(BattleInfo);
		}

		return;
	}
	UnitActorManager->Attack(this, AttackStruct, EAttackerType::AttackerTypeBuilding);
}

void ABuilding::InitAttacks()
{
	if (!AttacksDataAsset)
	{
		return;
	}
	AttackComponent->SetAttacks(AttacksDataAsset->Attacks);
	AttackComponent->SetStarted(true);
}

void ABuilding::LifeTimeRemainingMulticast_Implementation(const float RemainingTime, const float InitialDuration)
{
	OnLifeTimeRemainingBP(RemainingTime, InitialDuration);
	if (!LifeTimeTransparencyCurve) return;

	const auto Factor = RemainingTime / BuildingInfo.LifeTime;
	const auto Mult = LifeTimeTransparencyCurve->GetFloatValue(Factor);
	BlinkDelay = Mult * BlinkBaseDelay;
}
