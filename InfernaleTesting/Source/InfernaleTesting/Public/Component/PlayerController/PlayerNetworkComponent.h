// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerControllerComponent.h"
#include "Components/ActorComponent.h"
#include "PlayerNetworkComponent.generated.h"

struct FPlayerInfo;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLaunchGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndOfTurn);

class AGameModeInfernale;
class AInfernalePawn;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UPlayerNetworkComponent : public UPlayerControllerComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerNetworkComponent();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void TrySpawnPlayer();
	
	UFUNCTION() void OnLaunchGame();

	UFUNCTION(Server, Reliable, BlueprintCallable) void PossessInfernalePawnServer();
	UFUNCTION(Server, Reliable) void SpawnPlayerServer();
	UFUNCTION(Server, Reliable) void NotifyPlayerReadyServer(FPlayerInfo PlayerInfo);

	UFUNCTION(Client, Reliable) void TakeOwnershipClient(AInfernalePawn* InfernalePawn);
	
	UFUNCTION(NetMulticast, Reliable) void OnLauchGameMulticast();

	UFUNCTION(BlueprintImplementableEvent) AInfernalePawn* SpawnPlayerBP();
	
	
public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FOnLaunchGame LaunchGame;
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FEndOfTurn EndOfTurn;

protected:
	AGameModeInfernale* GameModeInfernale;
	
};
