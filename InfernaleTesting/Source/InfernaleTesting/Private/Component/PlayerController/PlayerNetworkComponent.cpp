// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PlayerController/PlayerNetworkComponent.h"

#include "GameMode/GameInstanceInfernale.h"
#include "GameMode/Infernale/GameModeInfernale.h"
#include "GameMode/Infernale/PlayerControllerInfernale.h"
#include "Player/InfernalePawn.h"

// Sets default values for this component's properties
UPlayerNetworkComponent::UPlayerNetworkComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerNetworkComponent::BeginPlay()
{
	Super::BeginPlay();

	// Check for network or local authority
	if (GetOwnerRole() == ROLE_Authority) GameModeInfernale = Cast<AGameModeInfernale>(GetWorld()->GetAuthGameMode());

	if (!PlayerControllerInfernale->IsLocalController()) return;
	auto GameInstance = PlayerControllerInfernale->GetGameInstance();
	NotifyPlayerReadyServer(Cast<UGameInstanceInfernale>(GameInstance)->PersonalPlayerInfo);
}

void UPlayerNetworkComponent::NotifyPlayerReadyServer_Implementation(FPlayerInfo PlayerInfo)
{
	PlayerControllerInfernale->SetPlayerInfo(PlayerInfo);
	GameModeInfernale->LaunchGame.AddDynamic(this, &UPlayerNetworkComponent::OnLaunchGame);
	SpawnPlayerServer();
	GameModeInfernale->NotifyPlayerReady(PlayerControllerInfernale, PlayerInfo);
}

void UPlayerNetworkComponent::SpawnPlayerServer_Implementation()
{
	TrySpawnPlayer();
}

void UPlayerNetworkComponent::PossessInfernalePawnServer_Implementation()
{
	auto InfernalePawn = PlayerControllerInfernale->GetInfernalePawn();
	if (!InfernalePawn) {
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UPlayerNetworkComponent::PossessInfernalePawnServer, 0.1f, false);
		return;
	}

	PlayerControllerInfernale->Possess(InfernalePawn);
}

void UPlayerNetworkComponent::TakeOwnershipClient_Implementation(AInfernalePawn* InfernalePawn)
{
	if (!InfernalePawn) return;
	InfernalePawn->SetOwner(PlayerControllerInfernale);
}

void UPlayerNetworkComponent::OnLaunchGame()
{
	OnLauchGameMulticast();
}

void UPlayerNetworkComponent::OnLauchGameMulticast_Implementation()
{
	LaunchGame.Broadcast();
}

void UPlayerNetworkComponent::TrySpawnPlayer()
{
	auto InfernalePawn = PlayerControllerInfernale->GetInfernalePawn();
	if (InfernalePawn) return;

	PlayerControllerInfernale->SetInfernalePawn(SpawnPlayerBP());
	PossessInfernalePawnServer();
}

// Called every frame
void UPlayerNetworkComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

