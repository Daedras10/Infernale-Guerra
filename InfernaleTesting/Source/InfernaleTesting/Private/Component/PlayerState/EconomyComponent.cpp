// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerState/EconomyComponent.h"

#include "Component/PlayerState/TransmutationComponent.h"
#include "DataAsset/EconomyDataAsset.h"
#include "DataAsset/TransmutationDataAsset.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "LD/Buildings/Building.h"
#include "LD/Buildings/MainBuilding.h"

// Sets default values for this component's properties
UEconomyComponent::UEconomyComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	// ...
}

void UEconomyComponent::AddSoulsServer_Implementation(AActor* Source, ESoulsGainCostReason SoulsGainReason, const float Amount)
{
	// Handle sources
	auto LastSouls = Souls;

	switch (SoulsGainReason)
	{
	case ESoulsGainCostReason::StructureGain:
		Souls += Amount; //GetCostWithMultiplier(SoulsGainReason);
		break;
	case ESoulsGainCostReason::NeutralCampReward:
		Souls += Amount * GetCostMultiplier(SoulsGainReason);
		break;
	case ESoulsGainCostReason::BuildingRecycled:
		Souls += Amount * GetCostMultiplier(SoulsGainReason);
		break;
	case ESoulsGainCostReason::BaseIncome:
		Souls += Amount;
		break;
	case ESoulsGainCostReason::TransmutationNodeRefund:
		Souls += Amount;
		break;
	case ESoulsGainCostReason::SoulBeaconReward:
		Souls += Amount;
		SoulsBeaconIncome += Amount;
		break;
	case ESoulsGainCostReason::DebugIncome:
		Souls += Amount;
		break;
	default:
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("EconomyCompnent : Unhandled SoulsGainCostReason %d"), static_cast<int>(SoulsGainReason)));
		return;
	}

	ReplicateSoulsClient(Souls);
	SoulsValueChanged.Broadcast(Souls, LastSouls);
	SoulsGained.Broadcast(Souls, LastSouls);
}

void UEconomyComponent::ReplicateSoulsClient_Implementation(const float Amount)
{
	Souls = Amount; 
}

void UEconomyComponent::RemoveSoulsServer_Implementation(const float Amount, const bool bInstantVisual)
{
	auto LastSouls = Souls;
	Souls -= Amount;
	Souls = FMath::Max(Souls, 0.f);
	if (bInstantVisual) SoulsDisplayed -= LastSouls-Souls;
	ReplicateSoulsClient(Souls);
	SoulsValueChanged.Broadcast(Souls, LastSouls);
	SoulsLost.Broadcast(Souls, LastSouls);
}

void UEconomyComponent::SetSoulsServer_Implementation(const float Amount)
{
	Souls = Amount;
	DisplaySoulsValueChange.Broadcast(Souls, Souls);
}

float UEconomyComponent::GetSouls() const
{
	return Souls;
}

float UEconomyComponent::GetSoulsDisplayed() const
{
	return SoulsDisplayed;
}

float UEconomyComponent::GetNodeCost(ETransmutationNodeType NodeType) const
{
	if (NodeType == ETransmutationNodeType::TransmutationNodeTypeNone) return 0;

	const auto PlayerController = Cast<APlayerControllerInfernale>(PlayerStateInfernale->GetPlayerController());
	const auto TransmutationComponent = PlayerController->GetTransmutationComponent();

	auto Cost = TransmutationComponent->GetCostNextNode(NodeType);
	return Cost;
}

void UEconomyComponent::AddBaseBuildingServer_Implementation(AMainBuilding* Building)
{
	AddBaseBuilding(Building);
}

void UEconomyComponent::RemoveBaseBuildingServer_Implementation(AMainBuilding* Building)
{
	RemoveBaseBuilding(Building);
}

float UEconomyComponent::GetBuildingCostWithMultiplier(const ESoulsGainCostReason SoulsGainReason,
	const FBuildingStruct BuildingStruct) const
{
	auto PC = PlayerStateInfernale->GetPlayerController();
	auto TransmutationComponent = Cast<APlayerControllerInfernale>(PC)->GetTransmutationComponent();

	
	switch (SoulsGainReason)
	{
		case ESoulsGainCostReason::BuildingConstruction:
			return TransmutationComponent->ApplyEffect(GetBuildingBuildCost(BuildingStruct) * GetCostMultiplier(SoulsGainReason), ENodeEffect::NodeEffectBuildingConstructionCost);
		case ESoulsGainCostReason::BuildingUpgrade:
			return GetBuildingUpgradeCost(BuildingStruct) * GetCostMultiplier(SoulsGainReason);
		default:
			return -1;
	}
}

float UEconomyComponent::GetNegativeSoulsAllowed() const
{
	return FMath::Abs(EconomyDataAsset->NegativeSoulsAllowedSandbox);
}

// Called every frame
void UEconomyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!PlayerStateInfernale) return;
	
	auto LerpSpeedVal = DeltaTime*LerpSpeed;
	SoulsDisplayed = FMath::Lerp(SoulsDisplayed, Souls, LerpSpeedVal);

	// Check if we are owner (and not autority) of the player state
	//bool bIsOwner = PlayerStateInfernale->GetLocalRole() == ROLE_Authority || PlayerStateInfernale->GetLocalRole() == ROLE_AutonomousProxy;
	//if (FMath::Abs(SoulsDisplayed - Souls) < 0.5f) SoulsDisplayed = Souls;
	
	if (bDebugSouls) GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, FString::Printf(TEXT("Souls: %f"), Souls));
	if (bDebugSoulsDisplayed) GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, FString::Printf(TEXT("SoulsDisplayed: %f"), SoulsDisplayed));
}



// Called when the game starts
void UEconomyComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!EconomyDataAsset) return;
	Souls = EconomyDataAsset->InitialSouls;
	auto GameInstance = Cast<UGameInstanceInfernale>(GetWorld()->GetGameInstance());
	if (GameInstance)
	{
		if (!GameInstance->bIsPhilosophicallyAccurateGame) Souls = EconomyDataAsset->InitialSoulsSandbox + FMath::Abs(EconomyDataAsset->NegativeSoulsAllowedSandbox); 
	}
	IncomeDelay = EconomyDataAsset->EconomyGainGlobalDelay;
	SoulsDisplayed = Souls;
	SoulsValueChanged.AddDynamic(this, &UEconomyComponent::OnSoulsChanged);
	if (!PlayerStateInfernale)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("PlayerStateInfernale not found in UEconomyComponent::BeginPlay, will retry"));
		auto TimerHandle = FTimerHandle();
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UEconomyComponent::RetryBeginPlay, 0.1f, false);
	}

	if (!PlayerStateInfernale->HasAuthority()) return;
	auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return;

	GameMode->LaunchGame.AddDynamic(this, &UEconomyComponent::OnLaunchGame);
}

float UEconomyComponent::GetCostWithMultiplier(const ESoulsGainCostReason SoulsGainReason) const
{
	FSoulsGainCostValues Values;
	bool bFound = false;
	for (const auto& Value : EconomyDataAsset->SoulsGainedValues)
	{
		if (Value.SoulsGainReason != SoulsGainReason) continue;

		Values = Value;
		bFound = true;
		break;
		
	}
	if (!bFound) return 0.f;

	switch (SoulsGainReason)
	{
		case ESoulsGainCostReason::StructureGain:
			/*GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Base: %f + (%f * %f * %f = %f"),
				Values.BaseValue, Values.ValuePerBuilding, MainBuildings.Num(), GetCostMultiplier(SoulsGainReason),
				(Values.ValuePerBuilding * MainBuildings.Num() * GetCostMultiplier(SoulsGainReason))));*/
			return Values.BaseValue + (Values.ValuePerBuilding * MainBuildings.Num() * GetCostMultiplier(SoulsGainReason));
		default:
			return Values.BaseValue;
	}
}

float UEconomyComponent::GetCostMultiplier(const ESoulsGainCostReason SoulsGainReason) const
{
	return 1;
}

float UEconomyComponent::GetBuildingBuildCost(const FBuildingStruct BuildingStruct) const
{
	int IdenticBuildings = 0;
	for (const auto& Building : Buildings)
	{
		if (Building->GetBuildingInfo().BuildingName == BuildingStruct.BuildingName) IdenticBuildings++;
	}
	return BuildingStruct.BuildingCost + BuildingStruct.BuildingCostPerUnit * IdenticBuildings;
}

float UEconomyComponent::GetBuildingUpgradeCost(const FBuildingStruct BuildingStruct) const
{
	int IdenticBuildings = 0;
	for (const auto& Building : Buildings)
	{
		if (Building->GetBuildingInfo().BuildingName != BuildingStruct.BuildingName) continue;
		if (!Building->IsFullyConstructed()) continue;
		if (Building->IsPermanent()) IdenticBuildings++;
	}
	return BuildingStruct.UpgradeCost + BuildingStruct.UpgradeCostPerUnit * IdenticBuildings;
}

void UEconomyComponent::RemoveSouls(const float Amount, const bool bInstantVisual)
{
	RemoveSoulsServer(Amount, bInstantVisual);
}

void UEconomyComponent::AddSouls(AActor* Source, ESoulsGainCostReason SoulsGainReason,
                                                      const float Amount)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AddSouls"));
	AddSoulsServer(Source, SoulsGainReason, Amount);
}

void UEconomyComponent::SetSouls(const float Amount)
{
	SetSoulsServer(Amount);
}

void UEconomyComponent::AddBaseBuilding(AMainBuilding* Building)
{
	const auto StateOwner = PlayerStateInfernale->GetOwnerInfo();
	const auto BuildingOwner = Building->GetOwner();

	if (BuildingOwner.Player != StateOwner.Player || BuildingOwner.Team != StateOwner.Team) return;
	MainBuildings.AddUnique(Building);
}

void UEconomyComponent::RemoveBaseBuilding(AMainBuilding* Building)
{
	MainBuildings.Remove(Building);
}

void UEconomyComponent::AddBuilding(ABuilding* Building)
{
	const auto StateOwner = PlayerStateInfernale->GetOwnerInfo();
	const auto BuildingOwner = Building->GetOwner();

	if (BuildingOwner.Player != StateOwner.Player || BuildingOwner.Team != StateOwner.Team) return;
	Buildings.AddUnique(Building);
	BuildingCostChanged.Broadcast(this);
}

void UEconomyComponent::RemoveBuilding(ABuilding* Building)
{
	Buildings.Remove(Building);
	BuildingCostChanged.Broadcast(this);
}

void UEconomyComponent::GainDebugIncome()
{
	GainDebugIncomeServer();
}

void UEconomyComponent::GainCheatIncome(const float Amount)
{
	GainCheatIncomeServer(Amount);
}

void UEconomyComponent::GainDebugIncomeServer_Implementation()
{
	AddSouls(this->GetOwner(), ESoulsGainCostReason::DebugIncome, 100.f);
}

void UEconomyComponent::GainCheatIncomeServer_Implementation(const float Amount)
{
	AddSouls(this->GetOwner(), ESoulsGainCostReason::DebugIncome, Amount);
}

void UEconomyComponent::OnSoulsChanged(const float NewSouls, const float LastSouls)
{
	SoulsDisplayed = FMath::Lerp(SoulsDisplayed, NewSouls, 0.1f);
}

void UEconomyComponent::OnLaunchGame()
{
	if (!EconomyDataAsset) return;
	GetWorld()->GetTimerManager().SetTimer(IncomeHandle, this, &UEconomyComponent::GainIncome, IncomeDelay, true);
}

void UEconomyComponent::RetryBeginPlay()
{
	PlayerStateInfernale = Cast<APlayerStateInfernale>(GetOwner());
	if (!PlayerStateInfernale)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("PlayerStateInfernale not found in UEconomyComponent::BeginPlay, will retry"));
		auto TimerHandle = FTimerHandle();
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UEconomyComponent::RetryBeginPlay, 0.1f, false);
	}

	if (!PlayerStateInfernale->HasAuthority()) return;
	auto GameMode = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return;

	GameMode->LaunchGame.AddDynamic(this, &UEconomyComponent::OnLaunchGame);
}

void UEconomyComponent::GainIncome()
{
	if (bDebugSouls) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GainSouls"));
	FSoulsGainCostValues MainBuildingIncome = FSoulsGainCostValues();
	
	for (const auto SoulsGain : EconomyDataAsset->SoulsGainedValues)
	{
		if (SoulsGain.SoulsGainReason == ESoulsGainCostReason::StructureGain) MainBuildingIncome = SoulsGain;
		if (SoulsGain.SoulsGainReason == ESoulsGainCostReason::BaseIncome) BaseIncome = SoulsGain;
	}

	auto MainBuildingMultiplier = MainBuildingIncome.BaseValue + MainBuildings.Num() * MainBuildingIncome.ValuePerBuilding;
	auto MainBuildingGain = MainBuildingMultiplier * MainBuildings.Num();

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("MainBuildingGain: %f (%d * %f)"), MainBuildingGain, MainBuildings.Num(), MainBuildingMultiplier));

	BuildingIncome += MainBuildingGain + BaseIncome.BaseValue;
	
	if (bDebugSouls) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("MainBuildingGain: %f (%d * %f)"), MainBuildingGain, MainBuildings.Num(), MainBuildingMultiplier));
	AddSouls(this->GetOwner(), ESoulsGainCostReason::StructureGain, MainBuildingGain);
	AddSouls(this->GetOwner(), ESoulsGainCostReason::BaseIncome, BaseIncome.BaseValue);
}

void UEconomyComponent::GainBaseIncome()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SyncSouls"));
	AddSouls(this->GetOwner(), ESoulsGainCostReason::BaseIncome, BaseIncome.BaseValue);
}
