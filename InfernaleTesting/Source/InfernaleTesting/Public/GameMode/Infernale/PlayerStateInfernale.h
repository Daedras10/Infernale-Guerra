// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/Ownable.h"
#include "Structs/SimpleStructs.h"
#include "PlayerStateInfernale.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerStateOwnerChanged, APlayerStateInfernale*, PlayerState, FOwner,
                                             Owner);

class UEconomyComponent;
struct FCategoryStruct;
class UBuildingListDataAsset;


// USTRUCT(Blueprintable)
// struct FAffecting
// {
// 	GENERATED_BODY()
//
// 	//UPROPERTY(BlueprintReadWrite, EditAnywhere) ENodeEffect Effect;
// 	/*
// 	 * affectingType: 0 - unit, 1 - building, 2 - flux
// 	 * affectingPerType
// 	 */
// };
/**
 * 
 */
UCLASS()
class INFERNALETESTING_API APlayerStateInfernale : public APlayerState
{
	GENERATED_BODY()

public:
	APlayerStateInfernale();

	UEconomyComponent* GetEconomyComponent();
	UFUNCTION(BlueprintPure, BlueprintCallable) TMap<EPlayerOwning, FString> GetPlayerOwningNameMap() const;

protected:
	virtual void BeginPlay() override;

	void TryWarnGameModeInfernale();

	UFUNCTION(NetMulticast, Reliable) void BroadCastPlayerStateOwnerChanged(FOwner NewOwner);
	UFUNCTION(NetMulticast, Reliable) void SetOwnerInfoMulticast(FOwner NewOwner);
	UFUNCTION(NetMulticast, Reliable) void SetPlayerNamesMulticast(const TArray<EPlayerOwning>& PlayerOwning, const TArray<FString>& PlayerNames);

public:
	UFUNCTION(BlueprintCallable)
	TArray<FCategoryStruct> GetBuildingListAvailable();
	
	void Init(const FOwner NewOwner);
	FOwner GetOwnerInfo();

	UFUNCTION(BlueprintCallable, BlueprintPure) FOwner GetOwnerInfoBP();

	UFUNCTION(Server, Reliable) void SetOwnerInfo(FOwner NewOwner);
	UFUNCTION(Server, Reliable) void SetPlayerNamesServer(const TArray<EPlayerOwning>& PlayerOwning, const TArray<FString>& PlayerNames);
	UFUNCTION(Server, Reliable) void AskResynchServer();

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable) FPlayerStateOwnerChanged PlayerStateOwnerChanged;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UEconomyComponent* EconomyComponent;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) UBuildingListDataAsset* GetBuildingListDataAsset;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bDebug = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FOwner OwnerInfo;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FString PlayerName = "Default";

	UPROPERTY(BlueprintReadWrite, EditAnywhere) TMap<EPlayerOwning, FString> PlayerOwningNameMap = TMap<EPlayerOwning, FString>();
};
