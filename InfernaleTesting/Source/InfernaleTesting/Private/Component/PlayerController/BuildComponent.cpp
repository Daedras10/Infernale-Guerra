// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerController/BuildComponent.h"

#include "Component/PlayerController/InteractionComponent.h"
#include "Component/PlayerController/UIComponent.h"
#include "Component/PlayerState/EconomyComponent.h"
#include "Component/PlayerState/TransmutationComponent.h"
#include "GameMode/Infernale/PlayerStateInfernale.h"
#include "Kismet/GameplayStatics.h"
#include "LD/Breach.h"
#include "LD/Buildings/Building.h"
#include "LD/Buildings/MainBuilding.h"
#include "Manager/UnitActorManager.h"

// Sets default values for this component's properties
UBuildComponent::UBuildComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerControllerInfernale->BuildBuilding.AddDynamic(this, &UBuildComponent::OnBuildBuilding);
}

void UBuildComponent::OnBuildBuilding(ABreach* Breach, FBuildingStruct Building)
{
	BuildBuildingServer(Breach, Building);
}

void UBuildComponent::Upgrade(ABuilding* Building, EInteractionType InteractionType)
{
	switch (InteractionType) {
	case EInteractionType::InteractionTypeNone:
		break;
	case EInteractionType::InteractionTypeClick:
		UpgradeServer(Building);
		break;
	case EInteractionType::InteractionTypeHover:
		PlayerControllerInfernale->GetUIComponent()->SoulsCostHovered(true, Building->GetBuildingInfo().UpgradeCost);
		break;
	case EInteractionType::InteractionTypeUnhover:
		PlayerControllerInfernale->GetUIComponent()->SoulsCostHovered(false, Building->GetBuildingInfo().UpgradeCost);
		break;
	}
}

void UBuildComponent::Recycle(ABuilding* Building, EInteractionType InteractionType)
{
	if (!Building) return;
	if (!Building->WasBuildingInfoSet())
	{
		FTimerHandle TimerHandle;
		PlayerControllerInfernale->GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, Building, InteractionType]() {
			Recycle(Building, InteractionType);
		}, 0.1f, false);
		return;
	}
	auto RecycleCost = Building->GetBuildingInfo().RecycleReturn;
	RecycleCost = PlayerControllerInfernale->GetTransmutationComponent()->ApplyEffect(RecycleCost, ENodeEffect::NodeEffectBuildingRecycleSouls);
	
	switch (InteractionType) {
	case EInteractionType::InteractionTypeNone:
		break;
	case EInteractionType::InteractionTypeClick:
		RecycleServer(Building);
		break;
	case EInteractionType::InteractionTypeHover:
		PlayerControllerInfernale->GetUIComponent()->SoulsCostHovered(true, -RecycleCost);
		break;
	case EInteractionType::InteractionTypeUnhover:
		PlayerControllerInfernale->GetUIComponent()->SoulsCostHovered(false, -RecycleCost);
		break;
	}
}

void UBuildComponent::Overclock(ABuilding* Building, EInteractionType InteractionType)
{
	switch (InteractionType) {
	case EInteractionType::InteractionTypeNone:
		break;
	case EInteractionType::InteractionTypeClick:
		OverclockServer(Building);
		break;
	case EInteractionType::InteractionTypeHover:
		PlayerControllerInfernale->GetUIComponent()->SoulsCostHovered(true, Building->GetBuildingInfo().OverclockCost);
		break;
	case EInteractionType::InteractionTypeUnhover:
		PlayerControllerInfernale->GetUIComponent()->SoulsCostHovered(false, Building->GetBuildingInfo().OverclockCost);
		break;
	}
}

void UBuildComponent::RecycleServer_Implementation(ABuilding* Building)
{
	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
	if (!PlayerState) return;
	if (!Building) return;
	if (Building->IsOverclocked()) return;
	const auto EconomyComponent = PlayerState->GetEconomyComponent();
	auto RecycleCost = Building->GetBuildingInfo().RecycleReturn;
	RecycleCost = PlayerControllerInfernale->GetTransmutationComponent()->ApplyEffect(RecycleCost, ENodeEffect::NodeEffectBuildingRecycleSouls);
	EconomyComponent->AddSouls(Building, ESoulsGainCostReason::BuildingRecycled, RecycleCost);
	Building->DestroyBuilding(false);
}

void UBuildComponent::UpgradeServer_Implementation(ABuilding* Building)
{
	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
	if (!PlayerState) return;
	const auto EconomyComponent = PlayerState->GetEconomyComponent();
	// Multipliers?
	auto Cost = Building->GetBuildingInfo().UpgradeCost;
	if (EconomyComponent->GetSouls() < Cost) return;
	EconomyComponent->RemoveSouls(Cost);
	//Building->SetOverclocked();
}

void UBuildComponent::OverclockServer_Implementation(ABuilding* Building)
{
	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
	if (!PlayerState) return;
	const auto EconomyComponent = PlayerState->GetEconomyComponent();
	// Multipliers?
	auto Cost = Building->GetBuildingInfo().OverclockCost;
	if (EconomyComponent->GetSouls() < Cost)
	{
		// const auto MainBuilding = Cast<AMainBuilding>(Building);
		// if (MainBuilding) OverclockedRefusedEventOwning(MainBuilding);
		// else 
		// {
		// 	GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Not enough souls to overclock and not a main building"));
		// 	return;
		// }
		return;
	}
	EconomyComponent->RemoveSouls(Cost);
	Building->SetOverclocked();
}

void UBuildComponent::OverclockMainBuildingServer_Implementation(AMainBuilding* OverclockMainBuilding)
{
	if (OverclockMainBuilding == nullptr) return;
	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
	if (!PlayerState) return;
	const auto EconomyComponent = PlayerState->GetEconomyComponent();
	const auto Cost = OverclockMainBuilding->GetOverclockCost();
	if (EconomyComponent->GetSouls() < Cost)
	{
		OverclockedRefusedEventOwning(OverclockMainBuilding);
		return;
	}
	EconomyComponent->RemoveSouls(Cost);
	
	OverclockMainBuilding->StartOverclock();
	OverclockStartedEventOwning(OverclockMainBuilding);
}

void UBuildComponent::OverclockReadyServer_Implementation(AMainBuilding* OverclockMainBuilding)
{
	OverclockReadyEventOwning(OverclockMainBuilding);
}

void UBuildComponent::CallBuildingEventClient_Implementation(bool BuildSuccess)
{
	if (BuildSuccess)
	{
		BuildBuilding.Broadcast();
		return;
	}
	CancelBuildBuilding.Broadcast();
}

void UBuildComponent::CallMainBuildingCostChangedEventClient_Implementation(AMainBuilding* MainBuilding, float Cost)
{
	MainBuildingCostChangedEvent.Broadcast(MainBuilding, Cost);
}

void UBuildComponent::OverclockStartedEventOwning_Implementation(AMainBuilding* OverclockMainBuilding)
{
	OverclockStartedEvent.Broadcast(OverclockMainBuilding);
}

void UBuildComponent::OverclockReadyEventOwning_Implementation(AMainBuilding* OverclockMainBuilding)
{
	OverclockReadyEvent.Broadcast(OverclockMainBuilding);
}

void UBuildComponent::BuildBuildingOnMBServer_Implementation(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo)
{
	if (!MainBuilding)
	{
		CallBuildingEventClient(false);
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("MainBuilding is null : BuildBuildingOnMBServer_Implementation (buildComponent)"));
		return;
	}
	auto BreachesAvailable = MainBuilding->GetBreachesAvailableForConstruction();
	if (BreachesAvailable.Num() == 0)
	{
		CallBuildingEventClient(false);
		BuildingNotConstructedEventOwning(MainBuilding, BuildingInfo.BuildingName);
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("No Breaches available : BuildBuildingOnMBServer_Implementation (buildComponent)"));
		return;
	}
	for (auto i = 0; i < BreachesAvailable.Num(); i++)
	{
		const auto Breach = BreachesAvailable[i];
		if (!Breach) continue;
		if (Breach->HasBuildingOnBreach()) continue;

		const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
		if (!PlayerState)
		{
			CallBuildingEventClient(false);
			BuildingNotConstructedEventOwning(MainBuilding, BuildingInfo.BuildingName);
			return;
		}

		const auto EconomyComponent = PlayerState->GetEconomyComponent();
		const auto Cost = MainBuilding->GetNextBuildingCost();
			//EconomyComponent->GetBuildingCostWithMultiplier(ESoulsGainCostReason::BuildingConstruction, BuildingInfo);
		
		if (EconomyComponent->GetSouls() < Cost)
		{
			GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Not enough blood to build (%f < %f)"), EconomyComponent->GetSouls(), Cost));
			CallBuildingEventClient(false);
			BuildingNotConstructedEventOwning(MainBuilding, BuildingInfo.BuildingName);
			return;
		}
		
		const FBuildingStruct BuildInfo = GetBuildingInfoWithParent(Breach, BuildingInfo);
		if (Breach->HasBuildingOnBreach()) Breach->DestroyBuildingOnBreach(false);
		
		const FActorSpawnParameters SpawnParams;
		const FTransform Transform = Breach->GetBuildingSpawningTransform();
		const TSubclassOf<AActor> SpawnClass = BuildingInfo.BuildingClass;
		
		if (SpawnClass == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("SpawnClass of %s is null"), *BuildingInfo.BuildingName));
			CallBuildingEventClient(false);
			return;
		}

		PlayerState->GetEconomyComponent()->RemoveSouls(Cost);
		UClass* ClassType = SpawnClass->GetDefaultObject()->GetClass();
		
		AActor* Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
		ABuilding* BuildingActor = Cast<ABuilding>(Actor);
		BuildingActor->SetTransmutationComponent(TWeakObjectPtr<UTransmutationComponent>(PlayerControllerInfernale->GetTransmutationComponent()));

		BuildingActor->ChangeOwner(Breach->GetOwner());
		Breach->SetBuildingOnBreach(BuildingActor);

		auto TransmutationComponent = PlayerControllerInfernale->GetTransmutationComponent();
		BuildingActor->SetBuildingInfo(BuildInfo, TransmutationComponent);
		BuildingActor->SetBreach(Breach);
		CallBuildingEventClient(true);

		const auto NewCost = MainBuilding->GetNextBuildingCost();
		CallMainBuildingCostChangedEventClient(MainBuilding, NewCost);
		BuildingConstructedEventOwning(MainBuilding, BuildingInfo.BuildingName);
		return;
	}
	GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("No Breaches available, reached end : BuildBuildingOnMBServer_Implementation (buildComponent)"));
	CallBuildingEventClient(false);
	BuildingNotConstructedEventOwning(MainBuilding, BuildingInfo.BuildingName);
}

void UBuildComponent::SellBuildingOnMBServer_Implementation(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo)
{
	auto GetBreachesWithBuilding = MainBuilding->GetBreachesWithBuilding(BuildingInfo.BuildingName);

	for (auto i = 0; i < GetBreachesWithBuilding.Num(); i++)
	{
		const auto Breach = GetBreachesWithBuilding[i];
		if (!Breach) continue;
		if (!Breach->HasBuildingOnBreach()) continue;
		const auto Building = Breach->GetBuildingOnBreach();
		const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
		if (!PlayerState) return;
		if (!Building) return;
		if (Building->IsOverclocked()) return;
		const auto EconomyComponent = PlayerState->GetEconomyComponent();
		auto RecycleCost = Building->GetBuildingInfo().RecycleReturn;
		RecycleCost = PlayerControllerInfernale->GetTransmutationComponent()->ApplyEffect(RecycleCost, ENodeEffect::NodeEffectBuildingRecycleSouls);
		EconomyComponent->AddSouls(Building, ESoulsGainCostReason::BuildingRecycled, RecycleCost);
		Building->DestroyBuilding(false);

		const auto NewCost = MainBuilding->GetNextBuildingCost();
		CallMainBuildingCostChangedEventClient(MainBuilding, NewCost);
		RecyleAllowedEvent.Broadcast(MainBuilding);

		return;
	}
}

void UBuildComponent::BuildingConstructedEventOwning_Implementation(AMainBuilding* MainBuilding,
	const FString& BuildingName)
{
	BuildingConstructedEvent.Broadcast(MainBuilding, BuildingName);
}

void UBuildComponent::BuildingNotConstructedEventOwning_Implementation(AMainBuilding* MainBuilding,
	const FString& BuildingName)
{
	BuildingNotConstructedEvent.Broadcast(MainBuilding, BuildingName);
}

void UBuildComponent::OverclockedRefusedEventOwning_Implementation(AMainBuilding* OverclockMainBuilding)
{
	OverclockedRefusedEvent.Broadcast(OverclockMainBuilding);
}

FBuildingStruct UBuildComponent::GetBuildingInfoWithParent(ABreach* Breach, FBuildingStruct Building)
{
	FBuildingStruct BuildingInfo = Building;
	if (!Breach->HasBuildingOnBreach()) return BuildingInfo;

	const auto BuildingOnBreach = Breach->GetBuildingOnBreach();
	if (BuildingOnBreach == nullptr) return BuildingInfo;

	if (BuildingOnBreach->IsFullyConstructed()) return BuildingInfo;
	
	BuildingInfo.ConstructionTime = FMath::Min(BuildingInfo.ConstructionTime, BuildingOnBreach->GetConstructionTime());
	return BuildingInfo;
}

void UBuildComponent::BuildBuildingServer_Implementation(ABreach* Breach, FBuildingStruct Building)
{
	if (Breach == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, TEXT("Breach is null : BuildBuildingServer_Implementation (buildComponent)"));
		CallBuildingEventClient(false);
        return;
    }

	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
	if (!PlayerState)
	{
		CallBuildingEventClient(false);
		return;
	}

	const auto EconomyComponent = PlayerState->GetEconomyComponent();
	const auto Cost = EconomyComponent->GetBuildingCostWithMultiplier(ESoulsGainCostReason::BuildingConstruction, Building);
	
	if (EconomyComponent->GetSouls() < Cost)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("Not enough blood to build (%f < %f)"), EconomyComponent->GetSouls(), Cost));
		CallBuildingEventClient(false);
		return;
	}
	
	const FBuildingStruct BuildingInfo = GetBuildingInfoWithParent(Breach, Building);
	if (Breach->HasBuildingOnBreach()) Breach->DestroyBuildingOnBreach(false);
	
	const FActorSpawnParameters SpawnParams;
	const FTransform Transform = Breach->GetBuildingSpawningTransform();
	const TSubclassOf<AActor> SpawnClass = Building.BuildingClass;
	
	if (SpawnClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, FString::Printf(TEXT("SpawnClass of %s is null"), *Building.BuildingName));
		CallBuildingEventClient(false);
		return;
	}

	PlayerState->GetEconomyComponent()->RemoveSouls(Cost);
	
	
	UClass* ClassType = SpawnClass->GetDefaultObject()->GetClass();
	
	AActor* Actor = GetWorld()->SpawnActor(ClassType, &Transform, SpawnParams);
	ABuilding* BuildingActor = Cast<ABuilding>(Actor);
	BuildingActor->SetTransmutationComponent(TWeakObjectPtr<UTransmutationComponent>(PlayerControllerInfernale->GetTransmutationComponent()));

	BuildingActor->ChangeOwner(Breach->GetOwner());
	Breach->SetBuildingOnBreach(BuildingActor);

	auto TransmutationComponent = PlayerControllerInfernale->GetTransmutationComponent();
	BuildingActor->SetBuildingInfo(BuildingInfo, TransmutationComponent);
	BuildingActor->SetBreach(Breach);
	CallBuildingEventClient(true);
}

void UBuildComponent::SwapBuildingForServer_Implementation(ABreach* Breach,
	FBuildingStruct BuildingInfo)
{
	Recycle(Breach->GetBuildingOnBreach(), EInteractionType::InteractionTypeClick);
	BuildBuildingServer(Breach, BuildingInfo);
}


// Called every frame
void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UBuildComponent::SetPermanentBuilding(ABuilding* Building, bool IsPermanent)
{
	if (Building == nullptr) return;
	if (!Building->HasAuthority()) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Building is not authority"));

	const auto PlayerState = Cast<APlayerStateInfernale>(PlayerControllerInfernale->PlayerState);
	if (!PlayerState) return;

	const auto EconomyComponent = PlayerState->GetEconomyComponent();
	const auto Cost = EconomyComponent->GetBuildingCostWithMultiplier(ESoulsGainCostReason::BuildingUpgrade, Building->GetBuildingInfo());
	
	if (EconomyComponent->GetSouls() < Cost) return;
	PlayerState->GetEconomyComponent()->RemoveSouls(Cost);
	
	Building->SetPermanentFromServer(IsPermanent);
}

void UBuildComponent::CallOverclockReadyEvent(AMainBuilding* OverclockMainBuilding)
{
	OverclockReadyServer(OverclockMainBuilding);
}

void UBuildComponent::EffectClickedBuilding(ABuilding* Building, EInteractionType InteractionType,
                                            EBuildInteractionType BuildInteractionType)
{
	switch (BuildInteractionType) {
	case EBuildInteractionType::BuildInteractionTypeNone:
		break;
	case EBuildInteractionType::BuildInteractionTypeBuild:
		break;
	case EBuildInteractionType::BuildInteractionTypeUpgrade:
		Upgrade(Building, InteractionType);
		break;
	case EBuildInteractionType::BuildInteractionTypeRecycle:
		Recycle(Building, InteractionType);
		break;
	case EBuildInteractionType::BuildInteractionTypeOverclock:
		Overclock(Building, InteractionType);
		break;
	}
}

void UBuildComponent::SwapBuildingFor(ABreach* Breach, FBuildingStruct BuildingInfo)
{
	SwapBuildingForServer(Breach, BuildingInfo);
}

AMainBuilding* UBuildComponent::AskGoToNextCity(bool Next, AMainBuilding* CurrentMainBuilding)
{
	const auto InteractionComponent = PlayerControllerInfernale->GetInteractionComponent();
	AMainBuilding* NewMainBuilding = nullptr;
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitActorManager::StaticClass(), OutActors);

	if (OutActors.Num() == 0) return nullptr;
	const AUnitActorManager* UnitActorManager = Cast<AUnitActorManager>(OutActors[0]);
	TArray<AMainBuilding*> MainBuildings = UnitActorManager->GetMainBuildings();

	int FoundId = -1;
	for (int i = 0; i < MainBuildings.Num(); i++)
    {
        if (MainBuildings[i] == nullptr) continue;
        if (MainBuildings[i] != CurrentMainBuilding) continue;
        FoundId = i;
        break;
    }
	if (FoundId == -1) return nullptr;
	const int Change = Next ? 1 : -1;
	int NewId = FoundId + Change;
	const auto CurrentBuildingInfo = CurrentMainBuilding->GetOwner();
	while (NewId != FoundId)
	{
		if (NewId >= MainBuildings.Num()) NewId = 0;
		if (NewId < 0) NewId = MainBuildings.Num() - 1;
		const auto OtherBuildingInfo = MainBuildings[NewId]->GetOwner();
		
		if (CurrentBuildingInfo.Player == OtherBuildingInfo.Player && CurrentBuildingInfo.Team == OtherBuildingInfo.Team) 
		{
			NewMainBuilding = MainBuildings[NewId];
			break;
		}
		NewId += Change;
	}
	if (NewId == FoundId) return nullptr;
	
	InteractionComponent->ForceSetMainInteract(NewMainBuilding);
	InteractionComponent->ForceStartMainInteract(NewMainBuilding, false);

	return NewMainBuilding;
}

void UBuildComponent::AskOverclock(AMainBuilding* OverclockMainBuilding)
{
	OverclockMainBuildingServer(OverclockMainBuilding);
}

void UBuildComponent::BuildBuildingOnMB(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo)
{
	BuildBuildingOnMBServer(MainBuilding, BuildingInfo);
}

void UBuildComponent::SellBuildingOnMB(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo)
{
	SellBuildingOnMBServer(MainBuilding, BuildingInfo);
}

