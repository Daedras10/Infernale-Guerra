// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerControllerComponent.h"
#include "Components/ActorComponent.h"
#include "BuildComponent.generated.h"


class AMainBuilding;
class FBuildBuilding;
class ABuilding;
struct FBuildingStruct;
class ABreach;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBuildBuildingDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMainBuildingDelegate, AMainBuilding*, MainBuilding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMainBuildingStringDelegate, AMainBuilding*, MainBuilding, FString, BuildingName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMainBuildingFloatDelegate, AMainBuilding*, MainBuilding, float, Cost);

UENUM(Blueprintable)
enum class EInteractionType : uint8
{
	InteractionTypeNone,
	InteractionTypeClick,
	InteractionTypeHover,
	InteractionTypeUnhover,
};

UENUM(Blueprintable)
enum class EBuildInteractionType : uint8
{
	BuildInteractionTypeNone,
	BuildInteractionTypeBuild,
	BuildInteractionTypeUpgrade,
	BuildInteractionTypeRecycle,
	BuildInteractionTypeOverclock,
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UBuildComponent : public UPlayerControllerComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBuildComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void SetPermanentBuilding(ABuilding* Building, bool IsPermanent);
	void CallOverclockReadyEvent(AMainBuilding* OverclockMainBuilding);

	UFUNCTION(BlueprintCallable) void EffectClickedBuilding(ABuilding* Building, EInteractionType InteractionType, EBuildInteractionType BuildInteractionType);
	UFUNCTION(BlueprintCallable) void SwapBuildingFor(ABreach* Breach, FBuildingStruct BuildingInfo);

	UFUNCTION(BlueprintCallable) AMainBuilding* AskGoToNextCity(bool Next, AMainBuilding* CurrentMainBuilding);
	UFUNCTION(BlueprintCallable) void AskOverclock(AMainBuilding* OverclockMainBuilding);

	UFUNCTION(BlueprintCallable) void BuildBuildingOnMB(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo);
	UFUNCTION(BlueprintCallable) void SellBuildingOnMB(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo);
	
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBuildBuilding(ABreach* Breach, FBuildingStruct Building);

	UFUNCTION(Server, Reliable) void BuildBuildingServer(ABreach* Breach, FBuildingStruct Building);
	UFUNCTION(Server, Reliable) void SwapBuildingForServer(ABreach* Breach, FBuildingStruct BuildingInfo);

	void Upgrade(ABuilding* Building, EInteractionType InteractionType);
	void Recycle(ABuilding* Building, EInteractionType InteractionType);
	void Overclock(ABuilding* Building, EInteractionType InteractionType);
	
	UFUNCTION(Server, Reliable) void UpgradeServer(ABuilding* Building);
	UFUNCTION(Server, Reliable) void RecycleServer(ABuilding* Building);
	UFUNCTION(Server, Reliable) void OverclockServer(ABuilding* Building);
	UFUNCTION(Server, Reliable) void OverclockMainBuildingServer(AMainBuilding* OverclockMainBuilding);
	UFUNCTION(Server, Reliable) void OverclockReadyServer(AMainBuilding* OverclockMainBuilding);

	UFUNCTION(Server, Reliable) void BuildBuildingOnMBServer(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo);
	UFUNCTION(Server, Reliable) void SellBuildingOnMBServer(AMainBuilding* MainBuilding, FBuildingStruct BuildingInfo);
	
	UFUNCTION(Client, Reliable) void CallBuildingEventClient(bool BuildSuccess);
	UFUNCTION(Client, Reliable) void CallMainBuildingCostChangedEventClient(AMainBuilding* MainBuilding, float Cost);
	UFUNCTION(Client, Reliable) void OverclockStartedEventOwning(AMainBuilding* OverclockMainBuilding);
	UFUNCTION(Client, Reliable) void OverclockReadyEventOwning(AMainBuilding* OverclockMainBuilding);

	UFUNCTION(Client, Reliable) void BuildingConstructedEventOwning(AMainBuilding* MainBuilding, const FString& BuildingName);
	UFUNCTION(Client, Reliable) void BuildingNotConstructedEventOwning(AMainBuilding* MainBuilding, const FString& BuildingName);
	UFUNCTION(Client, Reliable) void OverclockedRefusedEventOwning(AMainBuilding* OverclockMainBuilding);

	FBuildingStruct GetBuildingInfoWithParent(ABreach* Breach, FBuildingStruct Building);
	
public:
	UPROPERTY(BlueprintAssignable) FBuildBuildingDelegate BuildBuilding;
	UPROPERTY(BlueprintAssignable) FBuildBuildingDelegate CancelBuildBuilding;
	UPROPERTY(BlueprintAssignable) FMainBuildingDelegate OverclockStartedEvent;
	UPROPERTY(BlueprintAssignable) FMainBuildingDelegate OverclockReadyEvent;
	UPROPERTY(BlueprintAssignable) FMainBuildingFloatDelegate MainBuildingCostChangedEvent;

	UPROPERTY(BlueprintAssignable) FMainBuildingStringDelegate BuildingConstructedEvent;
	UPROPERTY(BlueprintAssignable) FMainBuildingStringDelegate BuildingNotConstructedEvent;
	UPROPERTY(BlueprintAssignable) FMainBuildingDelegate OverclockedRefusedEvent;

	UPROPERTY(BlueprintAssignable) FMainBuildingDelegate RecyleAllowedEvent;

	UPROPERTY(BlueprintAssignable) FMainBuildingDelegate CostErrorEvent;
};
