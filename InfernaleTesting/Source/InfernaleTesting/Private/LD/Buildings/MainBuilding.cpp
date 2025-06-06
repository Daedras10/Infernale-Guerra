// Fill out your copyright notice in the Description page of Project Settings.


#include "LD/Buildings/MainBuilding.h"

#include "NavigationSystem.h"
#include "Component/ActorComponents/AttackComponent.h"
#include "Component/ActorComponents/DamageableComponent.h"
#include "Component/ActorComponents/SpawnerComponent.h"
#include "Component/GameModeComponent/VictoryManagerComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "DataAsset/AttacksDataAsset.h"
#include "DataAsset/BuildingEffectDataAsset.h"
#include "DataAsset/EconomyDataAsset.h"
#include "DataAsset/FluxSettingsDataAsset.h"
#include "DataAsset/GameSettingsDataAsset.h"
#include "DataAsset/MainBuildingDataAsset.h"
#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Kismet/KismetMathLibrary.h"
#include "LD/Breach.h"
#include "LD/Buildings/Building.h"
#include "LD/LDElement/Boss.h"
#include "Manager/UnitActorManager.h"
#include "Mass/Collision/SpatialHashGrid.h"
#include "Component/ActorComponents/BattleManagerComponent.h"
#include "Component/PlayerController/BuildComponent.h"
#include "Mass/Amalgam/Traits/AmalgamTraitBase.h"

AMainBuilding::AMainBuilding()
{
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	DamageableComponent->SetCaptureMode(true);
}

void AMainBuilding::SetOwner(FOwner NewOwner)
{
	Super::SetOwner(NewOwner);

	if (!HasAuthority()) return;

	TimerHandle_BloodGain.Invalidate();
	if (NewOwner.Player == EPlayerOwning::Nature) return;
	
	//GetWorld()->GetTimerManager().SetTimer(TimerHandle_BloodGain, this, &AMainBuilding::GainBlood, TimeBeforeBloodGain, true);
}

void AMainBuilding::ChangeOwner(FOwner NewOwner)
{
	if (!HasAuthority()) return;
	if (!GameModeInfernaleWasInitialized)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("GameModeInfernale not initialized in MainBuilding, ChangeOwner"));
		return;
	}
	if (GameModeInfernale->IsGameStarted())
	{
		const auto PlayerState = GameModeInfernale->GetPlayerState(OwnerWithTeam.Player);
		if (PlayerState.IsValid()) PlayerState->GetEconomyComponent()->RemoveBaseBuilding(this);
	}
	Super::ChangeOwner(NewOwner);
	OwnershipChanged();
	
	ResetFluxes();
}

bool AMainBuilding::CanCreateAFlux() const
{
	return true;
}

AMainBuilding* AMainBuilding::GetMainBuilding()
{
	return this;
}

void AMainBuilding::OnSelectedForFluxCpp()
{
	ShowControlArea(true);
}

void AMainBuilding::OnDeselectedForFluxCpp()
{
	ShowControlArea(false);
}

void AMainBuilding::UpdateMaxHealth(float NewMaxHealth)
{
	RefreshMaxHealth();
}

bool AMainBuilding::IsMainBuilding() const
{
	return true;
}

float AMainBuilding::GetThornDamage() const
{
	const auto IsNotNature = OwnerWithTeam.Player != EPlayerOwning::Nature;
	return IsNotNature ? PlayerThornDamage : NeutralThornDamage;
}

float AMainBuilding::GetVictoryPointValue() const
{
	return VictoryPointValue;
}

TArray<ABreach*> AMainBuilding::GetBreaches() const
{
	return Breaches;
}

bool AMainBuilding::StartOwned()
{
	return bStartOwned;
}

void AMainBuilding::AskRefreshMaxHealth()
{
	RefreshMaxHealth();
}

void AMainBuilding::RandomizeBreachesOnGameStart()
{
	const auto RandomBreachOnStart = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->RandomBreachOnStart;
	if (RandomBreachOnStart) SpawnCityDistrictsV2();
	UpdateBreachesMulticast(Breaches);

	//RandomizeBreaches(false);
	for (auto Breach : Breaches)
	{
		if (!Breach) {
			continue;
		}
		BuildingOwnershipChanged.AddDynamic(Breach, &ABreach::MainBuildingOwnershipChanged);
		Breach->SetMainBuilding(this);
		//UE_LOG(LogTemp, Warning, TEXT("MainBuilding %s: Breach %s init"), *GetName(), *Breach->GetName());
	}
	ChangeOwner(OwnerWithTeam);
}

void AMainBuilding::StartOrCreateTimerIfNotExists()
{
	if (TimerHandle_BossPercentageGains.IsValid())
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_BossPercentageGains, this, &AMainBuilding::BossPercentageGains, 1.f, true);
}

float AMainBuilding::GetControlAreaRadius() const
{
	return ActualControlAreaRadius;
}

void AMainBuilding::AddVictoryPointValue(float Value)
{
}

void AMainBuilding::ShowControlArea(bool bShow)
{
	if (bShow == bAreaVisible) return;
	bAreaVisible = bShow;
	ShowControlAreaMulticast(bShow);

	if (bShow)
	{
		GameModeInfernale->MaxRadiusChanged.AddDynamic(this, &AMainBuilding::OnMaxRadiusChanged);
		return;
	}
	GameModeInfernale->MaxRadiusChanged.RemoveDynamic(this, &AMainBuilding::OnMaxRadiusChanged);
}

void AMainBuilding::ApplyBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UseOverclockEffect)
{
	auto StructEffect = FEffectStruct();
	StructEffect.Effects.Add(BuildingEffect.Type, BuildingEffect);
	StructEffect.bOverclocked = UseOverclockEffect;
	EffectsPerSource.Add(Source, StructEffect);

	switch (BuildingEffect.Type) {
	case EBuildingEffectType::BuildingEffectTypeNone:
		break;
	case EBuildingEffectType::BuildingEffectTypeHealth:
		RefreshMaxHealth();
		break;
	case EBuildingEffectType::BuildingEffectTypeRange:
		RefreshControlRange();
		break;
	case EBuildingEffectType::BuildingEffectTypeSummonBoss:
		RefreshSummonBossEffect();
		break;
	case EBuildingEffectType::BuildingEffectTypeFlux:
		CreateNewFluxesEffect(Source, BuildingEffect);
		break;
	case EBuildingEffectType::BuildingEffectTypeAttack:
		break;
	}
}

void AMainBuilding::RemoveBuildingEffect(ABuilding* Source, FBuildingEffect BuildingEffect, bool UpdateVisual)
{
	EffectsPerSource.Remove(Source);

	/* RefreshAll */
	RefreshControlRange();
	RefreshMaxHealth();
	RefreshSummonBossEffect();
}

void AMainBuilding::InteractStartHover(APlayerControllerInfernale* Interactor)
{
	if (Interactor->bIsEscapeMenuOpen) return;
	Super::InteractStartHover(Interactor);
	if (Interactor->GetPlayerOwning() != OwnerWithTeam.Player) return;
	const auto AllFluxes = GetFluxes();
	for (auto Flux : AllFluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->SimpleHoverFlux();
	}
}

void AMainBuilding::InteractEndHover(APlayerControllerInfernale* Interactor)
{
	Super::InteractEndHover(Interactor);
	const auto AllFluxes = GetFluxes();
	for (auto Flux : AllFluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->SimpleUnHoverFlux();
	}
}

FString AMainBuilding::GetBuildingName()
{
	return MainBuildingName;
}

void AMainBuilding::CreateNewFluxesEffect(ABuilding* Source, FBuildingEffect BuildingEffect)
{
	const auto GameSettingsDataAssetRef = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	const auto FluxSettingsDataAsset = GameSettingsDataAssetRef->DataAssetsSettings[GameSettingsDataAsset->DataAssetsSettingsToUse].FluxSettings;
	const auto FluxClass = FluxSettingsDataAsset->FluxSpawnClass;
	UClass* ClassType = FluxClass->GetDefaultObject()->GetClass();
	FActorSpawnParameters SpawnParams;

	const auto AllFluxes = GetFluxes();
	if (!MainBuildingDataAsset) return;

	const auto NewFluxesToCreate = BuildingEffect.Value;

	/* Initialize values */
	int NewFluxesCreated = 0;
	auto AngleDegree2 = StartingAngleDegree;
	auto AngleRad2 = FMath::DegreesToRadians(AngleDegree2);
	
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OutNavLoc;
	
	auto MainBuildingLoc = GetActorLocation();
	MainBuildingLoc.Z = 0;
	int Offset = 0;
	bool TargetReachable = false;
	
	while (Offset < 190 && NewFluxesCreated < NewFluxesToCreate)
	{
	 	AngleRad2 = FMath::DegreesToRadians(AngleDegree2 + Offset);
	 	auto OffsetFromCenterDir = FVector(FMath::Cos(AngleRad2), FMath::Sin(AngleRad2), 0);
	 	OffsetFromCenterDir.Normalize();
	 	auto PotentialPos = MainBuildingLoc + OffsetFromCenterDir * OffsetRange;
		
	 	TargetReachable = NavSys->ProjectPointToNavigation(PotentialPos, OutNavLoc, FVector(20, 20, 100));

	 	auto IsTooClose = false;
	 	if (TargetReachable)
     	{
	        for (auto Flux : AllFluxes)
	        {
		        if (!Flux.IsValid()) continue;
		        const auto FluxFirstNode = Flux->GetFirstNode();
		        if (!FluxFirstNode.IsValid()) continue;
		        const auto FluxNodeLoc = FluxFirstNode->GetActorLocation();
		        const auto Distance = (PotentialPos - FluxNodeLoc).Size();
		        if (Distance < TooCloseRadius)
		        {
		        	IsTooClose = true;
		        	break;
		        }
	        }
     	}
	 	
	 	FColor Color = TargetReachable ? (IsTooClose ? FColor::Purple : FColor::Green) : FColor::Red;
	 	if (bDebugFluxCreationPos) DrawDebugSphere(GetWorld(), PotentialPos, TooCloseRadius, 10, Color, false, 5.f, 0, 5.f);
	 	
	 	if (!IsTooClose)
	 	{
	 		const auto Transform = FTransform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(1, 1, 1));
	 		const auto Actor = GetWorld()->SpawnActor(ClassType, &Transform);
	 		const auto Flux = Cast<AFlux>(Actor);
	 		if (!Flux) continue;
	
	 		Flux->InitAsInactiveFlux(this, PotentialPos);
	 		Source->AddFlux(Flux);
	 		NewFluxesCreated++;
	 	}

	 	if (Offset == 0) Offset = 10;
	 	else if (Offset < 0) Offset = Offset * -1 + 10;
	 	else Offset *= -1;
	}
	UpdateSpawners();
}

void AMainBuilding::AddSpawnerComponents(USpawnerComponent* Spawner)
{
	SpawnerComponents.Add(TWeakObjectPtr<USpawnerComponent>(Spawner));
	UpdateSpawners();
}

void AMainBuilding::RemoveSpawnerComponents(USpawnerComponent* Spawner)
{
	for (auto i = SpawnerComponents.Num() - 1; i >= 0; i--)
	{
		auto SpawnerComponent = SpawnerComponents[i];
		if (!SpawnerComponent.IsValid())
		{
			SpawnerComponents.Remove(SpawnerComponent);
			continue;
		}
		if (SpawnerComponent.Get() == Spawner) SpawnerComponents.Remove(SpawnerComponent);
	}
	UpdateSpawners();
}

void AMainBuilding::AskUpdateSpawners()
{
	UpdateSpawners();
}

FVector AMainBuilding::GetFluxMidAngleNormalized() const
{
	const auto StartingAngleRad = FMath::DegreesToRadians(StartingAngleDegree);
	auto Dir = FVector(FMath::Cos(StartingAngleRad), FMath::Sin(StartingAngleRad), 0);
	Dir.Normalize();

	return Dir;
}

float AMainBuilding::GetAngleToleranceFlux() const
{
	const auto HalfAngleDegree = AngleAroundForFluxes / 2;
	const auto MaxDotAllowed = FMath::Cos(FMath::DegreesToRadians(HalfAngleDegree));
	return MaxDotAllowed;
}

float AMainBuilding::GetStartingAngle() const
{
	return StartingAngleDegree;
}

float AMainBuilding::GetAngleAroundForFluxes() const
{
	return AngleAroundForFluxes;
}

int AMainBuilding::NumberOfFluxes() const
{
	return NumberOfFluxInGame;
}

float AMainBuilding::GetTooCloseRadius() const
{
	return TooCloseRadius;
}

void AMainBuilding::StartOverclock()
{
	for (const auto Breach : Breaches)
	{
		if (!Breach) continue;
		if (!Breach->HasBuildingOnBreach()) continue;
		const auto BuildingOnBreach = Breach->GetBuildingOnBreach();
		if (!BuildingOnBreach) continue;
		if (!BuildingOnBreach->IsFullyConstructed()) continue;
		BuildingOnBreach->SetOverclocked();
	}

	AllowedToOverclock = false;
	SetAllowedToOverclockMulticast(false);

	FTimerHandle TimerHandle_Overclock;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_Overclock, this, &AMainBuilding::AllowToOverclock, OverclockCD, false);
	StartOverclockCDMulticast(true);
}

UBattleManagerComponent* AMainBuilding::GetBattleManager()
{
	if (BattleManager) return BattleManager;

	if (!UnitActorManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitActorManager::StaticClass(), OutActors);

		if (OutActors.Num() == 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("AmalgamFightProcessor : Unable to find UnitActorManager, skipping execution."));
			return nullptr;
		}
		UnitActorManager = static_cast<AUnitActorManager*>(OutActors[0]);
	}

	BattleManager = UnitActorManager->GetBattleManagerComponent().Get();

	return BattleManager;
}

void AMainBuilding::DisplayUI(bool Display)
{
	const auto WasDisplaying = UIisOpened;
	if (Display == WasDisplaying)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("DisplayUI : UI already %s" ), (Display ? TEXT("opened") : TEXT("closed"))));
		return;
	}
	Execute_DisplayWidget(this, Display, EDisplayWidgetType::DisplayWidgetMainBuilding);
	UIisOpened = Display;
}

TArray<TWeakObjectPtr<AFlux>> AMainBuilding::GetFluxes()
{
    TArray<TWeakObjectPtr<AFlux>> AllFluxes = TArray<TWeakObjectPtr<AFlux>>(Fluxes);
    for (const auto EffectPerSource : EffectsPerSource)
    {
        ABuilding* Source = Cast<ABuilding>(EffectPerSource.Key);
        if (!Source) continue;

        const auto FluxesFromSource = Source->GetLocalFluxes();
        for (const auto Flux : FluxesFromSource)
        {
            if (!Flux.IsValid()) continue;
            AllFluxes.Add(Flux);
        }
    }
    return AllFluxes;
}

int AMainBuilding::GetBreachesConstructedCount()
{
	int Count = 0;
	for (auto Breach : Breaches)
	{
		if (!Breach) continue;
		const auto BuildingOnBreach = Breach->GetBuildingOnBreach();
		if (!BuildingOnBreach) continue;
		Count++;
	}
	return Count;
}

TArray<float> AMainBuilding::GetActivatingFluxPower()
{
	auto FluxPowerDemon = 0.f;
	auto FluxPowerBuilding = 0.f;
	auto FluxPowerMonster = 0.f;

	auto FluxesNum = 1; // Start at one to account for activating flux
	const auto& AllFluxes = GetFluxes();
	for (const auto& Flux : AllFluxes)
	{
		if (!Flux.IsValid()) continue;
		if (!Flux.Get()->IsFluxActive()) continue;
		FluxesNum++;
	}

	for (const auto& SpawnerComponent : SpawnerComponents)
	{
		if (!SpawnerComponent.IsValid()) continue;

		const auto& AmalgamTrait = SpawnerComponent->GetAmalgamTrait();
		if (AmalgamTrait)
		{
			FluxPowerDemon += AmalgamTrait->PowerAgainstDemons;
			FluxPowerBuilding += AmalgamTrait->PowerAgainstBuildings;
			FluxPowerMonster += AmalgamTrait->PowerAgainstMonsters;
		}
	}

	if (FluxesNum > 0)
	{
		FluxPowerDemon /= FluxesNum;
		FluxPowerBuilding /= FluxesNum;
		FluxPowerMonster /= FluxesNum;
	}
	
	TArray<float> Output;
	Output.Add(FluxPowerDemon / FluxesNum);
	Output.Add(FluxPowerBuilding / FluxesNum);
	Output.Add(FluxPowerMonster / FluxesNum);
	return Output;
}

void AMainBuilding::CheatCapture(FOwner NewOwner)
{
	OnCaptureCompleted(NewOwner);
}

void AMainBuilding::BeginPlay()
{
	Super::BeginPlay();

	Type = EUnitTargetType::UTargetBuilding;

	DamageableComponent->CaptureDamaged.AddDynamic(this, &AMainBuilding::OnCaptureDamaged);
	DamageableComponent->CaptureCompleted.AddDynamic(this, &AMainBuilding::OnCaptureCompleted);
	DamageableComponent->CaptureCancelled.AddDynamic(this, &AMainBuilding::OnCaptureCancelled);

	for (auto Breach : Breaches)
	{
		if (!Breach) {
			continue;
        }
		BuildingOwnershipChanged.AddDynamic(Breach, &ABreach::MainBuildingOwnershipChanged);
		Breach->SetMainBuilding(this);
		//UE_LOG(LogTemp, Warning, TEXT("MainBuilding %s: Breach %s init"), *GetName(), *Breach->GetName());
	}

	Breaches.Remove(nullptr); /* LD cleanup */
	
	ChangeOwner(OwnerWithTeam);
	SyncDataAssets();
	
	bUseMass = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->UseMass;
	const auto AllFluxes = GetFluxes();
	for (const auto FluxRef : AllFluxes)
	{
		if (!FluxRef.IsValid()) continue;
		FluxRef->SetOrigin(this);
	}

	NumberOfFluxInGame = GetFluxes().Num();
	if (!HasAuthority()) return;
	
	BuildingParentFluxesUpdated.AddDynamic(this, &AMainBuilding::OnBuildingParentFluxesUpdated);
	AttackComponent->AttackReadyDelegate.AddDynamic(this, &AMainBuilding::OnAttackReady);

	SetLocalSoulsTimer();
	TryGetGameModeInfernale();
	FTimerHandle TimerHandle_InitAttacks;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_InitAttacks, this, &AMainBuilding::InitAttacks, 0.1f, false);

	for (const auto FluxRef : AllFluxes)
	{
		if (!FluxRef.IsValid()) continue;
		FluxRef.Get()->FluxEnabled.AddDynamic(this, &AMainBuilding::OnFluxEnabled);
	}
}

void AMainBuilding::OnHealthDepletedOwner(AActor* Actor, FOwner Depleter)
{
	Super::OnHealthDepletedOwner(Actor, Depleter);
	if (!HasAuthority()) return;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Building Health Depleted by Owner %d"), static_cast<int>(Depleter.Player)+1));

	ChangeOwner(Depleter);
}

void AMainBuilding::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority()) CalculateFluxesActiveCount();
	if (bDebugAngles) ShowAnglesTick();
	if (bIsOverclockedInCD)
	{
		OverclockCurrentTime += DeltaSeconds;
		if (OverclockCurrentTime >= OverclockDuration && bIsOverclocked)
		{
			bIsOverclocked = false;
			OverclockCurrentTime = 0.f;
		}
		if (OverclockCurrentTime >= OverclockCD)
		{
			bIsOverclockedInCD = false;
			OverclockCurrentTime = 0.f;
		}
	}

	if (!HasAuthority()) return;
	CaptureCurrentTime += DeltaSeconds;
	if (CaptureCurrentTime < CaptureRefreshRate) return;
	
	CaptureCurrentTime = 0.f;
	if (LastPercent >= 0.f && LastCapturedTeam != ETeam::NatureTeam)
	{
		CaptureDamageMulticast(LastCapturedTeam, LastPercent);
	}
	LastCapturedTeam = ETeam::NatureTeam;
	LastPercent = -1.f;
}

void AMainBuilding::GameModeInfernaleInitialized()
{
	GameModeInfernale->GetVictoryManagerComponent()->AddMainBuilding(this);
	GameModeInfernale->PreLaunchGame.AddDynamic(this, &AMainBuilding::OnPreLaunchGame);
	GameModeInfernale->LaunchGame.AddDynamic(this, &AMainBuilding::OnLaunchGame);
	GameModeInfernale->AllBasesAssigned.AddDynamic(this, &AMainBuilding::OnAllBasesAssigned);
	GameModeInfernaleWasInitialized = true;
}

void AMainBuilding::SyncDataAssets()
{
	if (!MainBuildingDataAsset) return;

	BaseMaxHealth = MainBuildingDataAsset->MaxHealth;
	BaseMaxHealthPlayer = MainBuildingDataAsset->MaxHealthPlayer;
	ControlAreaRadius = MainBuildingDataAsset->ControlAreaRadius;
	VictoryPointValue = MainBuildingDataAsset->VictoryPointValue;
	OffsetRange = MainBuildingDataAsset->OffsetRange;
	AttackOffsetRange = MainBuildingDataAsset->AttackOffsetRange;
	RepulsorRange = MainBuildingDataAsset->RepulsorRange;
	TargetableRange = MainBuildingDataAsset->TargetableRange;
	BuildingPriceCurve = MainBuildingDataAsset->BuildingPriceCurve;
	PlayerThornDamage = MainBuildingDataAsset->PlayerThornDamage;
	NeutralThornDamage = MainBuildingDataAsset->NeutralThornDamage;
	OverclockCD = MainBuildingDataAsset->OverclockCD;
	OverclockCost = MainBuildingDataAsset->OverclockCost;
	TooCloseRadius = MainBuildingDataAsset->TooCloseRadius;

	ActualMaxHealth = BaseMaxHealth;
	ActualControlAreaRadius = ControlAreaRadius;
	
	DamageableComponent->SetMaxHealthKeepPercent(ActualMaxHealth);
	DamageableComponent->SetHealingAllowed(true, MainBuildingDataAsset->Healing, MainBuildingDataAsset->HealingDelay, MainBuildingDataAsset->HealingDelaySinceLastAttack);
}

void AMainBuilding::TryGetGameModeInfernale()
{
	if (GameModeInfernaleWasInitialized) return;
	const auto World = GetWorld();
	GameModeInfernale = Cast<AGameModeInfernale>(World->GetAuthGameMode());
	
	if (!GameModeInfernale)
	{
		FTimerHandle TimerHandle_GameModeInfernale;
		World->GetTimerManager().SetTimer(TimerHandle_GameModeInfernale, this, &AMainBuilding::TryGetGameModeInfernale, 0.1f, false);
		return;
	}
	
	GameModeInfernaleInitialized();
}

void AMainBuilding::InitAttacks()
{
	if (!AttacksDataAsset)
	{
		return;
	}
	AttackComponent->SetAttacks(AttacksDataAsset->Attacks);
	AttackComponent->SetStarted(true);
}

void AMainBuilding::RefreshSummonBossEffect()
{
	auto NewBossAdd = 0.f;

	for (auto Effect : EffectsPerSource)
	{
		for (auto BuildingEffect : Effect.Value.Effects)
		{
			if (BuildingEffect.Value.Type != EBuildingEffectType::BuildingEffectTypeSummonBoss) continue;
			const auto BossEffect = Effect.Value.bOverclocked ? BuildingEffect.Value.ValueOverclocked : BuildingEffect.Value.Value;
			NewBossAdd += BossEffect;
		}
	}
	if (Boss) Boss->UpdateChargeAddedPerTick(BossPercentageToAdd, NewBossAdd);
	BossPercentageToAdd = NewBossAdd;
	StartOrCreateTimerIfNotExists();
}

void AMainBuilding::RefreshControlRange()
{
    ActualControlAreaRadius = ControlAreaRadius;
	auto Mults = 0;
	auto Adds = 0;
	
    for (auto Effect : EffectsPerSource)
    {
        for (auto BuildingEffect : Effect.Value.Effects)
        {
            if (BuildingEffect.Value.Type != EBuildingEffectType::BuildingEffectTypeRange) continue;
            const auto RangeEffect = Effect.Value.bOverclocked ? BuildingEffect.Value.ValueOverclocked : BuildingEffect.Value.Value;
        	if (BuildingEffect.Value.bIsPercent) Mults += RangeEffect;
        	else Adds += RangeEffect;
        }
    }
	
	ActualControlAreaRadius += Adds;
	ActualControlAreaRadius *= 1 + Mults;
	if (OwnerWithTeam.Player != EPlayerOwning::Nature && LocalTransmutationComponent.IsValid())
		ActualControlAreaRadius = LocalTransmutationComponent->ApplyEffect(ActualControlAreaRadius, ENodeEffect::NodeEffectFluxRange);
	//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Control Area Radius from %f to %f"), ControlAreaRadius, ActualControlAreaRadius));
	OnMaxRadiusChanged();
}

void AMainBuilding::RefreshMaxHealth()
{
	const auto IsNotNature = OwnerWithTeam.Player != EPlayerOwning::Nature;
	auto NewHealth = IsNotNature ? BaseMaxHealthPlayer : BaseMaxHealth;
	if (NewHealth <= 0.f) NewHealth = BaseMaxHealth;
	auto Mults = 0;
	auto Adds = 0;
	
	for (auto Effect : EffectsPerSource)
	{
		for (auto BuildingEffect : Effect.Value.Effects)
		{
			if (BuildingEffect.Value.Type != EBuildingEffectType::BuildingEffectTypeHealth) continue;
			const auto RangeEffect = Effect.Value.bOverclocked ? BuildingEffect.Value.ValueOverclocked : BuildingEffect.Value.Value;
			if (BuildingEffect.Value.bIsPercent) Mults += RangeEffect;
			else Adds += RangeEffect;
		}
	}
	NewHealth += Adds;
	NewHealth *= 1 + Mults;
	if (IsNotNature && LocalTransmutationComponent.IsValid())
		NewHealth = LocalTransmutationComponent->ApplyEffect(NewHealth, ENodeEffect::NodeEffectHealthBuilding);

	ActualMaxHealth = NewHealth;
	//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Max Health to %f"), NewHealth));
	DamageableComponent->SetMaxHealthKeepPercent(NewHealth);
}

void AMainBuilding::UpdateTransmutationFromOwner()
{
	if (!HasAuthority()) return;
	if (OwnerWithTeam.Player != EPlayerOwning::Nature) return;
	LocalTransmutationComponent = UnitActorManager->GetTransmutationComponent(OwnerWithTeam.Player);
	
}

void AMainBuilding::RefreshVisibilityLocal()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("RefreshVisibilityLocal"));
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("RefreshVisibilityLocal"));
	auto PlayerController = Cast<APlayerControllerInfernale>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PlayerController)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AMainBuilding::RefreshVisibilityLocal, .5f, false);
		return;
	}
	auto PlayerControllerOwner = PlayerController->GetOwnerInfo();
	if (PlayerControllerOwner.Player == EPlayerOwning::Nature)
    {
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AMainBuilding::RefreshVisibilityLocal, .5f, false);
        return;
    }
	
	for (auto Flux : GetFluxes())
	{
		if (!Flux.IsValid()) continue;
		Flux.Get()->RefreshNodeVisibility();
	}
}

void AMainBuilding::TryGainBlood()
{
	GainBlood();
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_BloodGain, this, &AMainBuilding::TryGainBlood, TimeBeforeBloodGain, false);
}

void AMainBuilding::GainBlood()
{
	if (!GameModeInfernaleWasInitialized) return;

	const auto PlayerState = GameModeInfernale->GetPlayerState(OwnerWithTeam.Player);
	if (!PlayerState.IsValid()) return; // PlayerState not found try to remove for optimization

	PlayerState->GetEconomyComponent()->AddSouls(this, ESoulsGainCostReason::StructureGain, 0);
}

void AMainBuilding::SetLocalSoulsTimer()
{
	if (!EconomyDataAsset) return;
	
	for (const auto SoulsGain : EconomyDataAsset->SoulsGainedValues)
	{
		if (SoulsGain.SoulsGainReason != ESoulsGainCostReason::StructureGain) continue;
		TimeBeforeBloodGain = SoulsGain.Time;
		break;
	}
}

void AMainBuilding::UpdateSpawners()
{
	auto FluxPowerDemon = 0.f;
	auto FluxPowerBuilding = 0.f;
	auto FluxPowerMonster = 0.f;
	
	auto FluxesNum = 0;
	auto AllFluxesExisting = 0;
	auto SpawnerComponentNum = 0;
	const auto AllFluxes = GetFluxes();
	for (auto Flux : AllFluxes)
	{
		if (!Flux.IsValid()) continue;
		AllFluxesExisting++;
		if (!Flux.Get()->IsFluxActive()) continue;
		FluxesNum++;
	}
	NumberOfFluxInGame = AllFluxesExisting;
	NumberOfFluxesInGameChanged(NumberOfFluxInGame);
	if (bDebugFluxSpawn) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Fluxes updated %d"), FluxesNum));
	if (bDebugFluxSpawn) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Updating %d spawners"), SpawnerComponents.Num()));
 
	const auto ShouldDisable = FluxesNum == 0;
	const auto Settings = UFunctionLibraryInfernale::GetGameSettingsDataAsset();
	const auto AllFluxSettings = Settings->DataAssetsSettings;
	if (AllFluxSettings.Num() <= Settings->DataAssetsSettingsToUse)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("GameSettingsDataAsset: No DataAssetsSettings for the index"));
		return;
	}
	const auto FluxSettings = Settings->DataAssetsSettings[Settings->DataAssetsSettingsToUse];
 
	float SpawnPerSeconds = 0;
	float Cumul = 0;
	for (auto SpawnerComponent : SpawnerComponents)
	{
		if (!SpawnerComponent.IsValid()) continue;
 
		SpawnerComponent->SetEnabled(!ShouldDisable);
		if (ShouldDisable) continue;
		//SpawnerComponent->OnFluxUpdated(this);
		const auto AmalgamTrait = SpawnerComponent->GetAmalgamTrait();
		if (AmalgamTrait)
		{
			FluxPowerDemon += AmalgamTrait->PowerAgainstDemons;
			FluxPowerBuilding += AmalgamTrait->PowerAgainstBuildings;
			FluxPowerMonster += AmalgamTrait->PowerAgainstMonsters;
		}
 
		const auto DefaultSpawnDelay = SpawnerComponent->GetDefaultSpawnDelay();
		const auto MultSpawnDelay = SpawnerComponent->GetSpawnDelayMult();
		SpawnerComponentNum++;
 
		SpawnPerSeconds += 1 / (DefaultSpawnDelay * FluxesNum * MultSpawnDelay);
		Cumul += 1 / (DefaultSpawnDelay * MultSpawnDelay);
	}
 
	if (ShouldDisable) return;
 
	/* Set flux speeds */
	auto AmalgamPerSeconds = Cumul;
	
	auto RatioSub = AmalgamPerSeconds;
	auto Ratio = RatioSub / FluxesNum;
	Ratio = FMath::Max(Ratio, 0.0f);

	if (FluxesNum > 0)
	{
		FluxPowerDemon /= FluxesNum;
		FluxPowerBuilding /= FluxesNum;
		FluxPowerMonster /= FluxesNum;
	}
	
	PowerDemon = FluxPowerDemon;
	PowerBuilding = FluxPowerBuilding;
	PowerMonster = FluxPowerMonster;

	for (auto Flux : AllFluxes)
    {
        if (!Flux.IsValid()) continue;
		if (!Flux.Get()->IsFluxActive()) continue;

		Flux->SetFluxPowers(FluxPowerDemon, FluxPowerBuilding, FluxPowerMonster, SpawnerComponentNum);
		
        const auto SpeedMult = FluxSettings.FluxSettings->SpeedRatioCurve->GetFloatValue(Ratio);
		Flux.Get()->SetAmalgamsSpeedMult(SpeedMult);
		if (bDebugFluxSpawn) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(
			TEXT("Flux speed %f  (ratio %f =  %f / %d | Cumuls = %f, AmalgamPerSeconds= %f)"),
			SpeedMult, Ratio, RatioSub, FluxesNum, Cumul, AmalgamPerSeconds));
    }
	
	const float MaxUnitsPerSeconds = UFunctionLibraryInfernale::GetGameSettingsDataAsset()->MaxUnitsPerSecondsPerBase;
	const bool TooManyUnits = SpawnPerSeconds > MaxUnitsPerSeconds;
	const float UnitMultiplier = TooManyUnits ? SpawnPerSeconds / MaxUnitsPerSeconds : 1;

	float Total = 0;
	for (auto SpawnerComponent : SpawnerComponents)
	{
		if (!SpawnerComponent.IsValid()) continue;
		
		const auto DefaultSpawnDelay = SpawnerComponent->GetDefaultSpawnDelay();
		const auto MultSpawnDelay = SpawnerComponent->GetSpawnDelayMult();
 
		const auto NewSpawnDelay = DefaultSpawnDelay * MultSpawnDelay * UnitMultiplier;
		SpawnerComponent->SetSpawnDelay(NewSpawnDelay);
		SpawnerComponent->SetSpawnerNumbers(SpawnerComponentNum);
		Total += NewSpawnDelay;
		SpawnerComponent->SetStrengthMultiplier(UnitMultiplier);
		if (bDebugFluxSpawn) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Delay %f ; SetStrengthMultiplier %f"), NewSpawnDelay, UnitMultiplier));
		SpawnerComponent->CallOnFluxUpdated();
	}
	const auto Nums = SpawnerComponents.Num();
	if (bDebugFluxSpawn) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Total %f => %f /sp, %f"), Total, Total/Nums, UnitMultiplier));
}

void AMainBuilding::BossPercentageGains_Implementation()
{
	if (!Boss)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Boss not found"));
		UE_LOG(LogTemp, Log, TEXT("Boss not found"));
		return;
	}
	//Boss->AddToSpawnCharge(BossPercentageToAdd);
}

void AMainBuilding::RefreshVisibilityMulticast_Implementation()
{
	RefreshVisibilityLocal();
}

void AMainBuilding::RandomizeBreaches(bool bResetPrevious)
{
	if (!MainBuildingDistrictsDataAsset) return;
	if (!BreachSpawnClass) return;
	if (bResetPrevious) SpawnCityDistricts();

	int BreachSpawned = 0;
	int BreachToSpawn = FMath::RandRange(MinBreachToSpawn, MaxBreachToSpawn);
	while (BreachSpawned < BreachToSpawn)
	{
		int RandomIndex = FMath::RandRange(0, (CityElements.InnerCity.Num() + CityElements.OuterCity.Num() - 1));
		if (RandomIndex < CityElements.InnerCity.Num())
		{
			FDistrictElement& District = CityElements.InnerCity[RandomIndex];
			if (District.IsBreach) continue;

			BreachSpawned++;
			District.IsBreach = true;
			District.Breach->SetIsBreach(true);
			auto Info = GetRandomInnerBreachInfo();
			District.Mesh = Info.DistrictMesh;
			District.Breach->SetMesh(Info.DistrictMesh);
			District.Scale = Info.Scale;
			District.ZOffset = Info.ZOffset;
			Breaches.Add(District.Breach);
			continue;
		}
		RandomIndex -= CityElements.InnerCity.Num();
		FDistrictElement& District = CityElements.OuterCity[RandomIndex];
		if (District.IsBreach) continue;

		BreachSpawned++;
		District.IsBreach = true;
		District.Breach->SetIsBreach(true);
		auto Info = GetRandomOuterBreachInfo();
		District.Mesh = Info.DistrictMesh;
		District.Breach->SetMesh(Info.DistrictMesh);
		District.Scale = Info.Scale;
		District.ZOffset = Info.ZOffset;
		Breaches.Add(District.Breach);
	}

	BreachesSpawnedMulticast(BreachSpawned);
}

void AMainBuilding::OwnershipChanged()
{
	const auto PlayerState = GameModeInfernale->GetPlayerState(OwnerWithTeam.Player);
	if (!PlayerState.IsValid()) return; // PlayerState not found try to remove for optimization
	
	PlayerState->GetEconomyComponent()->AddBaseBuilding(this);
	UpdateTransmutationFromOwner();
	//TryGainBlood();
}

void AMainBuilding::ClearFluxes()
{
	for (const auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		RemoveFluxDelegate(Flux);
		Flux->RemoveFluxServer();
	}
	Fluxes.Empty();
}

void AMainBuilding::ResetFluxes()
{
	for (const auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->ResetFluxServer();
	}
}

ABreach* AMainBuilding::SpawnBreachAt(int AngleIndex, int MaxAngleIndex, float Radius, UStaticMesh* Mesh, FVector Scale, float ZOffset)
{
	if (!BreachSpawnClass) return nullptr;
	UClass* BreachClass = BreachSpawnClass->GetDefaultObject()->GetClass();
	FActorSpawnParameters SpawnParams;
	const auto Angle = 2 * PI / MaxAngleIndex * AngleIndex;
	const auto X = Radius * FMath::Cos(Angle);
	const auto Y = Radius * FMath::Sin(Angle);
	const auto Offset = FVector(X, Y, ZOffset);
	auto Position = GetActorLocation() + Offset;
	auto Rotation = FRotator(0, FMath::RadiansToDegrees(Angle), 0);
	const auto Transform = FTransform(Rotation, Position, Scale);
	const auto Actor = GetWorld()->SpawnActor(BreachClass, &Transform, SpawnParams);
	const auto Breach = Cast<ABreach>(Actor);
	if (!Breach) return nullptr;
	
	Breach->SetOwner(OwnerWithTeam);
	Breach->SetMesh(Mesh);
	Breach->UpdateVisuals();
	Breach->SetMainBuilding(this);
	
	return Breach;
}

void AMainBuilding::CreateFluxesOnPreLaunchGame()
{
	SpawnFluxes();
	TArray<AFlux*> FluxesToSynch;
	for (const auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->SetOrigin(this);
		FluxesToSynch.Add(Flux.Get());
	}

	const auto AllFluxes = GetFluxes();
	for (const auto FluxRef : AllFluxes)
	{
		if (!FluxRef.IsValid()) continue;
		FluxRef->SetOrigin(this);
	}
	SychronizeFluxesMulticast(FluxesToSynch);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AMainBuilding::RefreshVisibilityLocal, .5f, false);

	if (!HasAuthority()) return;
	
	for (const auto FluxRef : AllFluxes)
	{
		if (!FluxRef.IsValid()) continue;
		FluxRef.Get()->FluxEnabled.AddDynamic(this, &AMainBuilding::OnFluxEnabled);
	}
}

void AMainBuilding::NumberOfFluxesInGameChanged(int NewNumberOfFluxesInGame)
{
	NumberOfFluxesInGameChangedMulticast(NewNumberOfFluxesInGame);
}

FDistrictElementProportion AMainBuilding::GetRandomInnerBreachInfo()
{
	FDistrictInfo InnerCity = MainBuildingDistrictsDataAsset->InnerCity;
	auto InnerBreaches = MainBuildingDistrictsDataAsset->InnerCity.BreachMeshes;
	
	float InnerBreachProportion = 0;
	for (const auto DistrictElement : InnerBreaches) InnerBreachProportion += DistrictElement.Proportion;
	
	float Random = FMath::FRand() * InnerBreachProportion;
	int Index = 0;
	for (int j = 0; j < InnerBreaches.Num(); j++)
	{
		Random -= InnerBreaches[j].Proportion;
		if (Random >= 0) Index++;
		else break;
	}
	return InnerBreaches[Index];
}

FDistrictElementProportion AMainBuilding::GetRandomOuterBreachInfo()
{
	FDistrictInfo OuterCity = MainBuildingDistrictsDataAsset->OuterCity;
	auto OuterBreaches = MainBuildingDistrictsDataAsset->OuterCity.BreachMeshes;

	float OuterBreachProportion = 0;
	for (const auto DistrictElement : OuterBreaches) OuterBreachProportion += DistrictElement.Proportion;

	float Random = FMath::FRand() * OuterBreachProportion;
	int Index = 0;
	for (int j = 0; j < OuterBreaches.Num(); j++)
	{
		Random -= OuterBreaches[j].Proportion;
		if (Random >= 0) Index++;
		else break;
	}
	return OuterBreaches[Index];
}

void AMainBuilding::OnPreLaunchGame()
{
	RefreshVisibilityLocal();
	RandomizeBreachesOnGameStart();
	
	ASpatialHashGrid::AddBuildingToGrid(GetActorLocation(), this);
	OwnershipChanged();

	auto BreachesNum = 0;
	for (auto Breach : Breaches)
	{
		if (!Breach) continue;
		Breach->SetMainBuilding(this);
		Breach->MainBuildingOwnershipChanged(this, OwnerWithTeam, OwnerWithTeam);
		BreachesNum++;
	}
	
	BreachesSpawnedMulticast(BreachesNum * 0.5);

	for (auto Breach : Breaches)
	{
		if (!Breach) continue;
		Breach->SetMainBuildingOnClients(this);
	}

	CreateFluxesOnPreLaunchGame();
}

void AMainBuilding::OnLaunchGame()
{
	/* Update Breach colors on LauchGame */
	for (auto Breach : Breaches)
	{
		if (!Breach) continue;
		Breach->UpdateVisuals();
	}
	RefreshMaxHealth();

	RefreshFluxVisibilityMulticast();
}

void AMainBuilding::OnAllBasesAssigned()
{
	UpdateTransmutationFromOwner();
	RefreshMaxHealth();
}

void AMainBuilding::OnAttackReady(FAttackStruct AttackStruct)
{
	if (bUseMass)
	{
		TMap<FMassEntityHandle, GridCellEntityData> Entities = ASpatialHashGrid::FindEntitiesInRange(GetActorLocation(), AttackStruct.AttackRange, AttackStruct.AttackAngle, GetActorForwardVector(), FMassEntityHandle(0, 0), OwnerWithTeam.Team);
		if (Entities.IsEmpty())
		{
			if (bDebug)
			{
				DebugAttackRange();
				// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("AMainBuilding::OnAttackReady !Entities.IsEmpty()"));
			}
			return;
		}
	
		TArray<FMassEntityHandle> Keys = TArray<FMassEntityHandle>();
		Entities.GenerateKeyArray(Keys);

		bool NoEnemies = true;
		for(const auto& Key : Keys)
		{
			if (ASpatialHashGrid::GetEntityData(Key).Owner.Team != OwnerWithTeam.Team)
			{
				ASpatialHashGrid::DamageEntity(Key, AttackStruct.AttackDamage);
				NoEnemies = false;
			}
		}

		if (NoEnemies) return;

		if (!BattleManager) GetBattleManager();

		if (BattleManager)
		{
			FBattleInfoCode BattleInfo = FBattleInfoCode();
			const FVector AttackLocation = GetActorLocation();
			BattleInfo.AttackerUnitType = EEntityType::EntityTypeCity;
			BattleInfo.TargetUnitType = EEntityType::EntityTypeNone; // Set as none because unit type is unknown
			BattleInfo.BattlePositionAttackerWorld = FVector2D(AttackLocation.X, AttackLocation.Y);
			BattleInfo.BattlePositionTargetWorld = FVector2D(0.f, 0.f); // Set at 0/0 because target position is irrelevent to aoe
			BattleInfo.AttackerOwner = OwnerWithTeam;
			BattleInfo.TargetOwner = FOwner(); // Set as uninitialized because there can be multiple owners
			BattleManager->AtPosBattleInfo(BattleInfo);
		}

		return;
	}
	UnitActorManager->Attack(this, AttackStruct, EAttackerType::AttackerTypeBuilding);
}

void AMainBuilding::OnMaxRadiusChanged()
{
	auto NewControlAreaRadius = GameModeInfernale->GetRadiusFromGameDuration(ActualControlAreaRadius);
	MaxRadiusMulticast(NewControlAreaRadius);
}

void AMainBuilding::OnCaptureDamaged(ETeam Team, float Percent)
{
	if (!HasAuthority()) return;
	//CaptureDamageMulticast(Team, Percent);

	LastCapturedTeam = Team;
	LastPercent = Percent;
}

void AMainBuilding::OnCaptureCompleted(FOwner DamageOwner)
{
	if (!HasAuthority()) return;
	OnHealthDepletedOwner(this, DamageOwner);

	LastCapturedTeam = ETeam::NatureTeam;
	LastPercent = -1.f;
	
	CaptureCompletedMulticast(DamageOwner);
}

void AMainBuilding::OnCaptureCancelled()
{
	if (!HasAuthority()) return;
	CaptureCancelledMulticast();
}

void AMainBuilding::AllowToOverclock()
{
	AllowedToOverclock = true;
	SetAllowedToOverclockMulticast(true);
	StartOverclockCDMulticast(false);

	const auto PlayerController = Cast<APlayerControllerInfernale>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PlayerController) return;
	PlayerController->GetBuildComponent()->CallOverclockReadyEvent(this);
}

void AMainBuilding::OnBuildingParentFluxesUpdated(ABuildingParent* _)
{
	UpdateSpawners();
}

void AMainBuilding::OnFluxEnabled(AFlux* Flux, bool bEnabled)
{
	UpdateSpawners();
}

void AMainBuilding::DebugBreach()
{
	FVector BaseLocation = GetActorLocation() + FVector(0, 0, 100);
	FVector BreachLocation;
	
	for (auto Breach : Breaches)
	{
		if (!Breach) continue;
		BreachLocation = Breach->GetActorLocation();
		DrawDebugLine(GetWorld(), BaseLocation, BreachLocation, FColor::Red, false, 5.f, 0, 50.f);
	}
}

void AMainBuilding::DebugAttackRange()
{
	DrawDebugSphere(GetWorld(), GetActorLocation(), AttacksDataAsset->Attacks[0].AttackRange, 12, FColor::Red, false, 5.f);
}

void AMainBuilding::InteractStartMain(APlayerControllerInfernale* Interactor)
{
	Super::InteractStartMain(Interactor);
	const auto InteractorPlayer = Interactor->GetPlayerOwning();
	const auto BuildingPlayer = OwnerWithTeam.Player;
	Interactor->MainBuildingInteracted(this);
	if (InteractorPlayer != BuildingPlayer)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("AMainBuilding::InteractStartMain: Interactor %s (player: %d) is not the same player as the building %s (%d)"), *Interactor->GetName(), InteractorPlayer, *GetName(), BuildingPlayer));
		return;
	}
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("AMainBuilding::InteractStartMain: Interactor %s (team: %d) is on the same team as the building %s (%d), %s"), *Interactor->GetName(), InteractorPlayer, *GetName(), BuildingPlayer, UIisOpened ? TEXT("UI already opened") : TEXT("UI not opened")));
	if (!UIisOpened) DisplayUI(true);
}

void AMainBuilding::InteractEndMain(APlayerControllerInfernale* Interactor)
{
	if (UIisOpened) DisplayUI(false);
	Super::InteractEndMain(Interactor);
}

bool AMainBuilding::InteractableHasUIOpen()
{
	return UIisOpened;
}

void AMainBuilding::SpawnBreaches()
{
	if (BreachesToSpawn <= 0) return;
	if (BreachSpawnClass == nullptr) return;
	
	UClass* BreachClass = BreachSpawnClass->GetDefaultObject()->GetClass();
	
	for (int i = 0; i < BreachesToSpawn; i++)
	{
		FActorSpawnParameters SpawnParams;
		const auto Angle = 2 * PI / BreachesToSpawn * i;
		const auto X = BreachSpawnRadius * FMath::Cos(Angle);
		const auto Y = BreachSpawnRadius * FMath::Sin(Angle);
		const auto Offset = FVector(X, Y, 0);
		auto Position = GetActorLocation() + Offset;
		auto Rotation = FRotator(0, FMath::RadiansToDegrees(Angle), 0);
		const auto Transform = FTransform(Rotation, Position, FVector(1, 1, 1));
		const auto Actor = GetWorld()->SpawnActor(BreachClass, &Transform, SpawnParams);
		const auto Breach = Cast<ABreach>(Actor);
		if (!Breach) continue;

		Breach->SetOwner(OwnerWithTeam);
		Breach->UpdateVisuals();
		Breach->SetMainBuilding(this);
		Breaches.Add(Breach);
	}
}

void AMainBuilding::DestroyLinkedBreaches()
{
	
	for (int i = Breaches.Num() - 1; i >= 0; i--)
	{
		const auto Breach = Breaches[i];
		if (!Breach) continue;
		Breach->Destroy();
	}
	Breaches.Empty();
}

void AMainBuilding::SpawnCityDistricts()
{
	if (!MainBuildingDistrictsDataAsset) return;
	if (!BreachSpawnClass) return;
	DestroyLinkedCityDistricts();
	
	FDistrictInfo InnerCity = MainBuildingDistrictsDataAsset->InnerCity;
	FDistrictInfo OuterCity = MainBuildingDistrictsDataAsset->OuterCity;
	auto InnerMeshes = MainBuildingDistrictsDataAsset->InnerCity.DistrictMeshes;
	auto OuterMeshes = MainBuildingDistrictsDataAsset->OuterCity.DistrictMeshes;
	auto InnerBreaches = MainBuildingDistrictsDataAsset->InnerCity.BreachMeshes;
	auto OuterBreaches = MainBuildingDistrictsDataAsset->OuterCity.BreachMeshes;

	float InnerDistrictProportion = 0;
	float OuterDistrictProportion = 0;
	float InnerBreachProportion = 0;
	float OuterBreachProportion = 0;

	for (const auto DistrictElement : InnerMeshes) InnerDistrictProportion += DistrictElement.Proportion;
	for (const auto DistrictElement : OuterMeshes) OuterDistrictProportion += DistrictElement.Proportion;
	for (const auto DistrictElement : InnerBreaches) InnerBreachProportion += DistrictElement.Proportion;
	for (const auto DistrictElement : OuterBreaches) OuterBreachProportion += DistrictElement.Proportion;

	float Random = 0;
	int Index = 0;

	for (int i = 0; i < InnerCityDistrictsToSpawn; i++)
	{
		Random = FMath::FRand() * InnerDistrictProportion;
		Index = 0;
		for (int j = 0; j < InnerMeshes.Num(); j++)
		{
			Random -= InnerMeshes[j].Proportion;
			if (Random >= 0) Index++;
			else break;
		}

		auto Info = InnerMeshes[Index];
		auto NewDistrict = Info.DistrictMesh;
		auto DistrictElement = FDistrictElement();
		DistrictElement.Mesh = NewDistrict;
		DistrictElement.IsBreach = false;
		DistrictElement.Scale = Info.Scale;
		DistrictElement.ZOffset = Info.ZOffset;
		DistrictElement.BuildingOffset = Info.BuildingOffset;
		
		DistrictElement.Breach = SpawnBreachAt(i, InnerCityDistrictsToSpawn, InnerCityDistrictsSpawnRadius, NewDistrict, Info.Scale, Info.ZOffset);
		DistrictElement.Breach->SetIsBreach(false);
		DistrictElement.Breach->SetBuildingSpawnOffset(Info.BuildingOffset);
		CityElements.InnerCity.Add(DistrictElement);
	}

	for (int i = 0; i < OuterCityDistrictsToSpawn; i++)
	{
		Random = FMath::FRand() * OuterDistrictProportion;
		Index = 0;
		for (int j = 0; j < OuterMeshes.Num(); j++)
		{
			Random -= OuterMeshes[j].Proportion;
			if (Random >= 0) Index++;
			else break;
		}

		auto Info = OuterMeshes[Index];
		auto NewDistrict = Info.DistrictMesh;
		auto DistrictElement = FDistrictElement();
		DistrictElement.Mesh = NewDistrict;
		DistrictElement.IsBreach = false;
		DistrictElement.Scale = Info.Scale;
		DistrictElement.ZOffset = Info.ZOffset;
		DistrictElement.BuildingOffset = Info.BuildingOffset;
		
		DistrictElement.Breach = SpawnBreachAt(i, OuterCityDistrictsToSpawn, OuterCityDistrictsSpawnRadius, NewDistrict, Info.Scale, Info.ZOffset);
		DistrictElement.Breach->SetIsBreach(false);
		DistrictElement.Breach->SetBuildingSpawnOffset(Info.BuildingOffset);
		CityElements.OuterCity.Add(DistrictElement);
	}
}

void AMainBuilding::DestroyLinkedCityDistricts()
{
	for (const auto DistrictElement : CityElements.InnerCity)
	{
		if (!DistrictElement.Breach) continue;
		DistrictElement.Breach->Destroy();
	}
	for (const auto DistrictElement : CityElements.OuterCity)
	{
		if (!DistrictElement.Breach) continue;
		DistrictElement.Breach->Destroy();
	}
	
	CityElements.InnerCity.Empty();
	CityElements.OuterCity.Empty();
}

void AMainBuilding::UpdateFromData()
{
	auto Z = GetActorLocation().Z;
	
	Breaches.Empty();
	for (int i = CityElements.InnerCity.Num() - 1; i >= 0; i--)
	{
		auto DistrictElement = CityElements.InnerCity[i];
		auto Breach = DistrictElement.Breach;
		if (!Breach)
		{
			CityElements.InnerCity.RemoveAt(i);
			continue;
		}
		Breach->SetMesh(DistrictElement.Mesh);
		Breach->SetActorScale3D(DistrictElement.Scale);
		Breach->SetActorLocation(Breach->GetActorLocation() + FVector(0, 0, DistrictElement.ZOffset));
		Breach->SetIsBreach(DistrictElement.IsBreach);
		Breach->SetBuildingSpawnOffset(DistrictElement.BuildingOffset);
		if (DistrictElement.IsBreach) Breaches.Add(Breach);
	}

	for (int i = CityElements.OuterCity.Num() - 1; i >= 0; i--)
	{
		auto DistrictElement = CityElements.OuterCity[i];
		auto Breach = DistrictElement.Breach;
		if (!Breach)
		{
			CityElements.OuterCity.RemoveAt(i);
			continue;
		}
		Breach->SetMesh(DistrictElement.Mesh);
		Breach->SetActorScale3D(DistrictElement.Scale);
		Breach->SetActorLocation(Breach->GetActorLocation() * FVector(1.0f, 1.0f, 0.f) + FVector(0, 0, Z+DistrictElement.ZOffset));
		Breach->SetIsBreach(DistrictElement.IsBreach);
		Breach->SetBuildingSpawnOffset(DistrictElement.BuildingOffset);
		if (DistrictElement.IsBreach) Breaches.Add(Breach);
	}
}

void AMainBuilding::RandomizeBreaches()
{
	if (!MainBuildingDistrictsDataAsset) return;
	if (!BreachSpawnClass) return;

	RandomizeBreaches(true);
}

void AMainBuilding::SpawnCityDistrictsV2()
{
	if (!MainBuildingDistrictsDataAsset) return;
	if (!BreachSpawnClass) return;
	DestroyLinkedCityDistrictsV2();
	
	auto BaseRotation = GetActorRotation(); //Set to 0 if bUseBaseRotation is false
	auto BaseLocation = GetActorLocation();

	const auto BaseSpawnInfo = MainBuildingDistrictsDataAsset->DistrictElementSpawnInfos;
	TArray<FDistrictElementSpawnInfo> Present = TArray<FDistrictElementSpawnInfo>();
	TArray<FDistrictElementSpawnInfo> CanSpawn = TArray<FDistrictElementSpawnInfo>();
	TArray<FString> PresentIds = TArray<FString>();
	

	/* SpawnRequiered */
	for (const auto SpawnInfo : BaseSpawnInfo)
	{
		if (!SpawnInfo.MustAlwaysSpawn) continue;
		Present.Add(SpawnInfo);
		PresentIds.Add(SpawnInfo.Id);
		if (bDebugTool) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Spawn Required"));
		SpawnBreachWithInfo(SpawnInfo, BaseLocation, BaseRotation);
	}

	const int ToSpawn = FMath::RandRange(MinBreachToSpawn, MaxBreachToSpawn);
	if (bDebugTool) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("To Spawn: %d"), ToSpawn));
	while (Present.Num() < ToSpawn)
	{
		/* Populate CanSpawn */
		CanSpawn.Empty();
		for (const auto SpawnInfo : BaseSpawnInfo)
		{
			bool bPresent = PresentIds.Contains(SpawnInfo.Id);
			if (bPresent) continue;

			if (!MainBuildingDistrictsDataAsset->AllowIsolatedBuildings)
			{
				bool OnePrerequisiteOk = false;
				if (SpawnInfo.Neighbors.Num() == 0) OnePrerequisiteOk = true;
				else
				{
					for (const auto Prerequisite : SpawnInfo.Neighbors)
					{
						if (PresentIds.Contains(Prerequisite))
						{
							OnePrerequisiteOk = true;
							break;
						}
					}
				}
				if (!OnePrerequisiteOk) continue;
			}
			CanSpawn.Add(SpawnInfo);
		}

		if (CanSpawn.Num() == 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No more Options to spawn"));
			break;
		}

		int RandomIndex = FMath::RandRange(0, CanSpawn.Num() - 1);
		auto SpawnInfo = CanSpawn[RandomIndex];
		Present.Add(SpawnInfo);
		PresentIds.Add(SpawnInfo.Id);
		if (bDebugTool) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Spawn %s"), *SpawnInfo.Id));
		auto Breach = SpawnBreachWithInfo(SpawnInfo, BaseLocation, BaseRotation);
		if (!Breach) break;
	}
}

void AMainBuilding::DestroyLinkedCityDistrictsV2()
{
	for (auto DistrictData : DistrictDataV2)
	{
		if (!DistrictData.Breach) continue;
		DistrictData.Breach->Destroy();
	}
	DistrictDataV2.Empty();
	Breaches.Empty();
}

void AMainBuilding::UpdateFromDataV2()
{
	const auto BaseRotation = GetActorRotation();
	const auto BaseLocation = GetActorLocation();
	
	Breaches.Empty();
	for (int i = DistrictDataV2.Num() - 1; i >= 0; i--)
	{
		const auto Info = DistrictDataV2[i].SpawnInfo;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Edit %s"), *Info.Id));
		auto Breach = DistrictDataV2[i].Breach;
		if (!Breach)
		{
			DistrictDataV2.RemoveAt(i);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Destroying %s"), *Info.Id));
			continue;
		}
		Breaches.Add(Breach);

		FVector OffsetFromMainBuilding = Info.OffsetFromMainBuilding;
		FVector SpawnLocation = BaseLocation + BaseRotation.RotateVector(OffsetFromMainBuilding);
		SpawnLocation += FVector(0, 0, Info.BreachOffsetZ);
		Breach->SetActorLocation(SpawnLocation);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("SpawnLocation %s"), *SpawnLocation.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("OffsetFromMainBuilding %s"), *OffsetFromMainBuilding.ToString()));
		Breach->SetActorRotation(BaseRotation); //UKismetMathLibrary::FindLookAtRotation(SpawnLocation, BaseLocation)

		auto Mesh = UFunctionLibraryInfernale::GetMeshFromId(Info.MeshId, GameSettingsDataAsset);
		if (Mesh) Breach->SetMesh(Mesh);

		Breach->SetOwner(OwnerWithTeam);
		Breach->SetIsBreach(true);
		Breach->UpdateVisuals();
		Breach->SetMainBuilding(this);
		Breach->SetBuildingSpawnOffset(FVector(0, 0, Info.BuildingOffsetZ));
	}
}

void AMainBuilding::SaveData()
{
	FString SaveStr = "";
	for (const auto DistrictData : DistrictDataV2)
	{
		const auto Info = DistrictData.SpawnInfo;
		/* 0 : ID */
		SaveStr += DistrictData.Id + ";";
		
		/* 1 : Neigbors */
		for (const auto Neighbor : Info.Neighbors)
		{
			SaveStr += Neighbor + ",";
		}
		if (SaveStr.EndsWith(",")) SaveStr = SaveStr.LeftChop(1);
		SaveStr += ";";

		/* 2 : OffsetFromMainBuilding */
		SaveStr += FString::SanitizeFloat(Info.OffsetFromMainBuilding.X) + ",";
		SaveStr += FString::SanitizeFloat(Info.OffsetFromMainBuilding.Y) + ",";
		SaveStr += FString::SanitizeFloat(Info.OffsetFromMainBuilding.Z) + ";";

		/* 3 : Mesh */
		SaveStr += Info.MeshId + ";";

		/* 4 : Scale */
		SaveStr += FString::SanitizeFloat(Info.Scale.X) + ",";
		SaveStr += FString::SanitizeFloat(Info.Scale.Y) + ",";
		SaveStr += FString::SanitizeFloat(Info.Scale.Z) + ";";

		/* 5 : ZOffset */
		SaveStr += FString::SanitizeFloat(Info.BreachOffsetZ) + ";";

		/* 6 : BuildingOffset */
		SaveStr += FString::SanitizeFloat(Info.BuildingOffsetZ) + ";";

		SaveStr += "|";
	}
	if (!SaveStr.IsEmpty()) SaveStr = SaveStr.LeftChop(1);
	SaveDataString = SaveStr;
}

void AMainBuilding::LoadData()
{
	TArray<FString> LoadedDatas = TArray<FString>();
	LoadFromSavedDataString.ParseIntoArray(LoadedDatas, TEXT("|"), true);
	DestroyLinkedCityDistrictsV2();

	const auto ActorLocation = GetActorLocation();
	const auto ActorRotation = GetActorRotation();

	if (bDebugTool) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("LoadedDatas Num: %d"), LoadedDatas.Num()));
	
	for (const auto LoadedData : LoadedDatas)
	{
		
		TArray<FString> Infos = TArray<FString>();
		if (bDebugTool) GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Green, FString::Printf(TEXT("LoadedData: %s"), *LoadedData));
		
		LoadedData.ParseIntoArray(Infos, TEXT(";"), false);
		if (bDebugTool)
		{
			for (int i = 0; i < Infos.Num(); i++)
			{
				GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Green, FString::Printf(TEXT("\tInfos[%d]: %s"), i, *Infos[i]));
			}
		}

		FDistrictElementSpawnInfo SpawnInfo;
		SpawnInfo.Id = Infos[0];

		if (!Infos[1].IsEmpty())
		{
			TArray<FString> Neighbors = TArray<FString>();
			Infos[1].ParseIntoArray(Neighbors, TEXT(","), true);
			SpawnInfo.Neighbors = Neighbors;
		}

		TArray<FString> OffsetFromMainBuildingStr = TArray<FString>();
		Infos[2].ParseIntoArray(OffsetFromMainBuildingStr, TEXT(","), true);
		if (OffsetFromMainBuildingStr.Num() != 3)
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("OffsetFromMainBuildingStr Num: %d"), OffsetFromMainBuildingStr.Num()));
			GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("OffsetFromMainBuildingStr: %s"), *Infos[2]));
			continue;
		}
		SpawnInfo.OffsetFromMainBuilding = FVector(FCString::Atof(*OffsetFromMainBuildingStr[0]), FCString::Atof(*OffsetFromMainBuildingStr[1]), FCString::Atof(*OffsetFromMainBuildingStr[2]));

		const auto MeshId = Infos[3];
		const auto Mesh = UFunctionLibraryInfernale::GetMeshFromId(MeshId, GameSettingsDataAsset);
		if (!Mesh)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Mesh %s not found"), *MeshId));
			continue;
		}
		SpawnInfo.MeshId = MeshId;

		TArray<FString> ScaleStr = TArray<FString>();
		Infos[4].ParseIntoArray(ScaleStr, TEXT(","), true);
		if (ScaleStr.Num() != 3)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ScaleStr Num: %d"), ScaleStr.Num()));
			continue;
		}
		SpawnInfo.Scale = FVector(FCString::Atof(*ScaleStr[0]), FCString::Atof(*ScaleStr[1]), FCString::Atof(*ScaleStr[2]));

		SpawnInfo.BreachOffsetZ = FCString::Atof(*Infos[5]);
		SpawnInfo.BuildingOffsetZ = FCString::Atof(*Infos[6]);

		if (bDebugTool) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Spawn %s"), *SpawnInfo.Id));
		SpawnBreachWithInfo(SpawnInfo, ActorLocation, ActorRotation);
	}
}

void AMainBuilding::ShowDebugID()
{
	for (auto DistrictData : DistrictDataV2)
	{
		if (!DistrictData.Id.Equals(DebugId)) continue;
		if (!DistrictData.Breach) continue;
		const auto DataLoc = DistrictData.Breach->GetActorLocation();
		auto Location = DataLoc + FVector(0, 0, 750);
		DrawDebugString(GetWorld(), Location, DistrictData.Id, this, FColor::Red, 5.f, true);
		DrawDebugSphere(GetWorld(), Location, 100, 10, FColor::Red, false, DebugDuration, 0, 5.f);
	}
}

void AMainBuilding::UpdateMainBuildingMesh()
{
	UpdateMainBuildingMeshBP();
}

TArray<USpawnerComponent*> AMainBuilding::GetSpawnerComponents()
{
	TArray<USpawnerComponent*> ConvertedComponents;
	for (auto Component : SpawnerComponents)
		ConvertedComponents.Add(Component.Get());

	return ConvertedComponents;
}

int AMainBuilding::GetFluxesActiveCount()
{
	return LastFluxActiveNumbers;
}

bool AMainBuilding::IsAllowedToOverclock()
{
	return AllowedToOverclock;
}

bool AMainBuilding::IsOverclockInCD()
{
	return bIsOverclockedInCD;
}

bool AMainBuilding::IsOverclocked()
{
	return bIsOverclocked;
}

float AMainBuilding::OverclockedCurrent()
{
	return OverclockCurrentTime;
}

float AMainBuilding::OverclockedCD()
{
	return OverclockCD;
}

float AMainBuilding::GetOverclockCost()
{
	return OverclockCost;
}

void AMainBuilding::CalculateFluxesActiveCount()
{
	if (!HasAuthority()) return;
	
	int FluxesActiveCount = 0;
	auto FluxesLocal = GetFluxes();
	for (const auto Flux : FluxesLocal)
	{
		if (!Flux.IsValid()) continue;
		if (Flux->IsFluxActive()) FluxesActiveCount++;
	}
	if (FluxesActiveCount != LastFluxActiveNumbers)
	{
		LastFluxActiveNumbers = FluxesActiveCount;
		ReplicateLastFluxActiveNumbersMulticast(FluxesActiveCount);
	}
}

TArray<ABreach*> AMainBuilding::GetBreachesAvailableForConstruction()
{
	TArray<ABreach*> BreachesAvailable;
	for (const auto Breach : Breaches)
	{
		if (!Breach) continue;
		if (Breach->HasBuildingOnBreach()) continue;
		BreachesAvailable.Add(Breach);
	}
	return BreachesAvailable;
}

TArray<ABreach*> AMainBuilding::GetBreachesWithBuilding(FString BuildingName)
{
	TArray<ABreach*> BreachesWithBuilding;
	for (const auto Breach : Breaches)
	{
		if (!Breach) continue;
		if (!Breach->HasBuildingOnBreach()) continue;
		TWeakObjectPtr<ABuilding> Building = TWeakObjectPtr<ABuilding>(Breach->GetBuildingOnBreach());
		if (!Building.IsValid()) continue;
		const auto BuidlingInfo = Building.Get()->GetBuildingInfo();
		if (!BuidlingInfo.BuildingName.Equals(BuildingName, ESearchCase::CaseSensitive)) continue;
		BreachesWithBuilding.Add(Breach);
	}
	return BreachesWithBuilding;
}

TArray<ABreach*> AMainBuilding::GetBreachesWithAnyBuilding()
{
	TArray<ABreach*> BreachesWithBuilding;
	for (const auto Breach : Breaches)
	{
		if (!Breach) continue;
		if (!Breach->HasBuildingOnBreach()) continue;
		TWeakObjectPtr<ABuilding> Building = TWeakObjectPtr<ABuilding>(Breach->GetBuildingOnBreach());
		if (!Building.IsValid()) continue;
		BreachesWithBuilding.Add(Breach);
	}
	return BreachesWithBuilding;
}

float AMainBuilding::GetNextBuildingCost()
{
	auto BuildingsExistings = GetBreachesWithAnyBuilding();
	return BuildingPriceCurve->GetFloatValue(BuildingsExistings.Num());
}

void AMainBuilding::ShowAnglesTick()
{
	const auto BaseLocation = GetActorLocation();
	const auto StartingAngleRad = FMath::DegreesToRadians(StartingAngleDegree);
	const auto LightOffset = FVector(0, 0, 200);

	if (!MainBuildingDataAsset) return;
	const auto FluxStartOffset = MainBuildingDataAsset->OffsetRange;
	
	auto OffsetFromCenterDir = FVector(FMath::Cos(StartingAngleRad), FMath::Sin(StartingAngleRad), 0);
	OffsetFromCenterDir.Normalize();
	auto AngleStartPos = BaseLocation + OffsetFromCenterDir * FluxStartOffset + LightOffset;
	auto AngleEndPos = AngleStartPos + OffsetFromCenterDir * 500.0f;
	DrawDebugDirectionalArrow(GetWorld(), AngleStartPos, AngleEndPos, 100.0f, FColor::Red, false, 0, 0, 20.f);

	/* AngleBefore */
	auto HalfAngle = AngleAroundForFluxes / 2;
	auto AngleRad = FMath::DegreesToRadians(StartingAngleDegree - HalfAngle);
	OffsetFromCenterDir = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0);
	OffsetFromCenterDir.Normalize();
	AngleStartPos = BaseLocation + OffsetFromCenterDir * FluxStartOffset + LightOffset;
	AngleEndPos = AngleStartPos + OffsetFromCenterDir * 500.0f;
	DrawDebugDirectionalArrow(GetWorld(), AngleStartPos, AngleEndPos, 100.0f, FColor::Cyan, false, 0, 0, 20.f);

	/* AngleAfter */
	AngleRad = FMath::DegreesToRadians(StartingAngleDegree + HalfAngle);
	OffsetFromCenterDir = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0);
	OffsetFromCenterDir.Normalize();
	AngleStartPos = BaseLocation + OffsetFromCenterDir * FluxStartOffset + LightOffset;
	AngleEndPos = AngleStartPos + OffsetFromCenterDir * 500.0f;
	DrawDebugDirectionalArrow(GetWorld(), AngleStartPos, AngleEndPos, 100.0f, FColor::Cyan, false, 0, 0, 20.f);
}

void AMainBuilding::ShowAngles()
{
	const auto BaseLocation = GetActorLocation();
	const auto StartingAngleRad = FMath::DegreesToRadians(StartingAngleDegree);
	const auto LightOffset = FVector(0, 0, 200);

	if (!MainBuildingDataAsset) return;
	const auto FluxStartOffset = MainBuildingDataAsset->OffsetRange;
	
	auto OffsetFromCenterDir = FVector(FMath::Cos(StartingAngleRad), FMath::Sin(StartingAngleRad), 0);
	OffsetFromCenterDir.Normalize();
    auto AngleStartPos = BaseLocation + OffsetFromCenterDir * FluxStartOffset + LightOffset;
	auto AngleEndPos = AngleStartPos + OffsetFromCenterDir * 500.0f;
	DrawDebugDirectionalArrow(GetWorld(), AngleStartPos, AngleEndPos, 100.0f, FColor::Red, false, DebugDuration, 0, 20.f);

	/* AngleBefore */
	auto HalfAngle = AngleAroundForFluxes / 2;
	auto AngleRad = FMath::DegreesToRadians(StartingAngleDegree - HalfAngle);
	OffsetFromCenterDir = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0);
	OffsetFromCenterDir.Normalize();
	AngleStartPos = BaseLocation + OffsetFromCenterDir * FluxStartOffset + LightOffset;
	AngleEndPos = AngleStartPos + OffsetFromCenterDir * 500.0f;
	DrawDebugDirectionalArrow(GetWorld(), AngleStartPos, AngleEndPos, 100.0f, FColor::Cyan, false, DebugDuration, 0, 20.f);

	/* AngleAfter */
	AngleRad = FMath::DegreesToRadians(StartingAngleDegree + HalfAngle);
	OffsetFromCenterDir = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0);
	OffsetFromCenterDir.Normalize();
	AngleStartPos = BaseLocation + OffsetFromCenterDir * FluxStartOffset + LightOffset;
	AngleEndPos = AngleStartPos + OffsetFromCenterDir * 500.0f;
	DrawDebugDirectionalArrow(GetWorld(), AngleStartPos, AngleEndPos, 100.0f, FColor::Cyan, false, DebugDuration, 0, 20.f);
}

void AMainBuilding::SpawnFluxes()
{
	if (!MainBuildingDataAsset) return;
	
	const auto HalfAngle = AngleAroundForFluxes / 2;
	const auto MinAngle = StartingAngleDegree - HalfAngle;
	const auto MaxAngle = StartingAngleDegree + HalfAngle;

	const auto FluxesRequired = NumberOfFlux;
	if (FluxesRequired <= 0) return;

	for (auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->RemoveFluxServer(); // Should clean up nodes ?
	}
	Fluxes.Empty();

	
	if (!GameSettingsDataAsset) return;
	const auto FluxSpawnClass = GameSettingsDataAsset->DataAssetsSettings[GameSettingsDataAsset->DataAssetsSettingsToUse].FluxSettings->FluxSpawnClass;
	UClass* ClassType = FluxSpawnClass->GetDefaultObject()->GetClass();
	

	for (int i = 0; i < FluxesRequired; i++)
	{
		auto AngleDegree = MinAngle + (MaxAngle - MinAngle) / FluxesRequired * i;
		auto AngleRad = FMath::DegreesToRadians(AngleDegree);
		auto OffsetFromCenterDir = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0);
		OffsetFromCenterDir.Normalize();
		const auto Position = GetActorLocation() + OffsetFromCenterDir * MainBuildingDataAsset->OffsetRange;
		const auto Transform = FTransform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(1, 1, 1));

		const auto Actor = GetWorld()->SpawnActor(ClassType, &Transform);
		const auto Flux = Cast<AFlux>(Actor);
		if (!Flux) continue;
		
		Flux->InitAsInactiveFlux(this, Position);
		AddFlux(Flux);
	}
}

void AMainBuilding::ClearFluxesTool()
{
	for (auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->RemoveFluxTool();
	}
	Fluxes.Empty();
}


ABreach* AMainBuilding::SpawnBreachWithInfo(FDistrictElementSpawnInfo Info, FVector BaseLocation, FRotator BaseRotation)
{
	if (!BreachSpawnClass) return nullptr;
	UClass* BreachClass = BreachSpawnClass->GetDefaultObject()->GetClass();
	FActorSpawnParameters SpawnParams;
	FVector OffsetFromMainBuilding = Info.OffsetFromMainBuilding;
	FVector SpawnLocation = BaseLocation + BaseRotation.RotateVector(OffsetFromMainBuilding);
	SpawnLocation += FVector(0, 0, Info.BreachOffsetZ);
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, BaseLocation);
	LookAtRotation = BaseRotation;
	FVector Scale = Info.Scale;
	
	const auto Transform = FTransform(LookAtRotation, SpawnLocation, Scale);
	const auto Actor = GetWorld()->SpawnActor(BreachClass, &Transform, SpawnParams);
	const auto Breach = Cast<ABreach>(Actor);
	if (!Breach) return nullptr;

	auto Mesh = UFunctionLibraryInfernale::GetMeshFromId(Info.MeshId, GameSettingsDataAsset);
	if (!Mesh)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Mesh %s not found"), *Info.MeshId));
		return nullptr;
	}
	
	Breach->SetOwner(OwnerWithTeam);
	Breach->SetMesh(Mesh);
	Breach->SetIsBreach(true);
	Breach->UpdateVisuals();
	Breach->SetMainBuilding(this);
	Breach->SetBuildingSpawnOffset(FVector(0, 0, Info.BuildingOffsetZ));
	Breaches.Add(Breach);

	FDistrictData DistrictData;
	DistrictData.Id = Info.Id;
	DistrictData.Breach = Breach;
	DistrictData.SpawnInfo = FDistrictElementSpawnInfo(Info);
	DistrictDataV2.Add(DistrictData);
	
	return Breach;
}

void AMainBuilding::NumberOfFluxesInGameChangedMulticast_Implementation(int NewNumberOfFluxesInGame)
{
	if (!HasAuthority()) NumberOfFluxInGame = NewNumberOfFluxesInGame;
	NumberOfFluxInGameUpdated.Broadcast(NewNumberOfFluxesInGame);
}

void AMainBuilding::ReplicateLastFluxActiveNumbersMulticast_Implementation(int NewLastFluxActiveNumbers)
{
	if (HasAuthority()) return;
	LastFluxActiveNumbers = NewLastFluxActiveNumbers;
}

void AMainBuilding::CaptureCancelledMulticast_Implementation()
{
	CaptureCancelledBP();
}

void AMainBuilding::StartOverclockCDMulticast_Implementation(const bool Overclocked)
{
	bIsOverclockedInCD = Overclocked;
	bIsOverclocked = Overclocked;
	OverclockCurrentTime = 0.0f;
}

void AMainBuilding::SetAllowedToOverclockMulticast_Implementation(const bool bAllowedToOverclock)
{
	AllowedToOverclock = bAllowedToOverclock;
}

void AMainBuilding::UpdateBreachesMulticast_Implementation(const TArray<ABreach*>& NewBreaches)
{
	if (HasAuthority()) return;
	Breaches.Empty();
	for (auto Breach : NewBreaches)
	{
		if (!Breach) continue;
		Breaches.Add(Breach);
	}
	BreachesUpdatedEvent.Broadcast(Breaches.Num());
}

void AMainBuilding::SychronizeFluxesMulticast_Implementation(const TArray<AFlux*>& NewFluxes)
{
	if (HasAuthority()) return;
	Fluxes.Empty();
	for (auto Flux : NewFluxes)
	{
		if (!Flux) continue;
		Fluxes.Add(TWeakObjectPtr<AFlux>(Flux));
		Flux->SetOrigin(this);
	}

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AMainBuilding::RefreshVisibilityLocal, .5f, false);
}

void AMainBuilding::RefreshFluxVisibilityMulticast_Implementation()
{
	for (const auto Flux : Fluxes)
	{
		if (!Flux.IsValid()) continue;
		Flux->ForceFluxActiveVisibility(false);
	}
}

void AMainBuilding::BreachesSpawnedMulticast_Implementation(int BreachesCount)
{
	BreachesSpawnedBP(BreachesCount);
}

void AMainBuilding::CaptureCompletedMulticast_Implementation(FOwner NewOwner)
{
	CaptureCompletedBP(NewOwner);
	DisplayUI(false);
}

void AMainBuilding::CaptureDamageMulticast_Implementation(ETeam Team, float Percent)
{
	CaptureDamageBP(Team, Percent);
}

void AMainBuilding::MaxRadiusMulticast_Implementation(float NewControlAreaRadius)
{
	MaxRadiusBP(NewControlAreaRadius);
}

void AMainBuilding::ShowControlAreaMulticast_Implementation(bool bShow)
{
	if (bShow) ShowControlAreaBP();
	else HideControlAreaBP();
}

