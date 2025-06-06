// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FogOfWar/FogOfWarComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/FluxRepulsor.h"
#include "Interfaces/InGameUI.h"
#include "Interfaces/Interactable.h"
#include "Interfaces/Ownable.h"
#include "Interfaces/VisuallyUpdatedByOwner.h"
#include "Structs/SimpleStructs.h"
#include "Breach.generated.h"

class AFogOfWarManager;
class AMainBuilding;
class ABuildingParent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBreachOwnershipChanged, ABreach*, Breach, FOwner, OldOwner, FOwner, NewOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBuildingBoolDelegate, ABuilding*, Building, bool, bBuildingOnBreach);

UCLASS()
class INFERNALETESTING_API ABreach : public AActor, public IOwnable, public IVisuallyUpdatedByOwner, public IInteractable, public IInGameUI, public IFluxRepulsor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABreach();
	
	virtual void Tick(float DeltaTime) override;
	virtual FOwner GetOwner() override;
	virtual void SetOwner(FOwner NewOwner) override;
	void UpdateFogOfWar(bool bRemove);

	UFUNCTION()
	void MainBuildingOwnershipChanged(ABuildingParent* Building, FOwner OldOwner, FOwner NewOwner);
	
	virtual void InteractStartMain(APlayerControllerInfernale* Interactor) override;
	virtual void InteractEndMain(APlayerControllerInfernale* Interactor) override;
	virtual void InteractStartHover(APlayerControllerInfernale* Interactor) override;
	virtual void InteractEndHover(APlayerControllerInfernale* Interactor) override;
	virtual bool ShouldEndMainInteractOnMove() override;
	virtual float GetRepulsorRange() const override;
	
	virtual void DisplayUI(bool Display) override;
	virtual bool InteractableHasUIOpen() override;

	FTransform GetBuildingSpawningTransform() const;
	bool HasBuildingOnBreach() const;
	void SetBuildingOnBreach(ABuilding* Building);
	void DestroyBuildingOnBreach(const bool bDestroyedByUnit = true);
	AMainBuilding* GetMainBuilding() const;
	void SetMainBuilding(AMainBuilding* NewMainBuilding);
	void SetMainBuildingOnClients(AMainBuilding* NewMainBuilding);

	UFUNCTION(BlueprintCallable)
	ABuilding* GetBuildingOnBreach() const;

	void LockBreach(const bool ShouldLock);
	void UpdateVisuals();
	void SetMesh(UStaticMesh* NewMesh);
	void SetIsBreach(bool bIsBreach);
	void SetBuildingSpawnOffset(FVector NewOffset);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breach")
	UFogOfWarComponent* FogOfWarComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION() void OnBuildingDestroyed(ABuilding* Building);
	UFUNCTION() void OnBuildingMadePermanent(ABuilding* Building);

	UFUNCTION(NetMulticast, Reliable) void LockBreachMulticast(const bool bLockedVal);
	UFUNCTION(NetMulticast, Reliable) void SetMeshMulticast(UStaticMesh* NewMesh);
	UFUNCTION(NetMulticast, Reliable) void SetBreachMulticast(const bool IsBreach);
	UFUNCTION(NetMulticast, Reliable) void SetOwnerWithTeamMulticast(const FOwner OwnerWithTeamVal);
	UFUNCTION(NetMulticast, Reliable) void SetBuildingOnBreachMulticast(ABuilding* BuildingOnBreachVal, bool bBuildingOnBreachVal);
	UFUNCTION(NetMulticast, Reliable) void SetMainBuildingMulticast(AMainBuilding* NewMainBuilding);

	UFUNCTION(BlueprintImplementableEvent) void OnHoverStart();
	UFUNCTION(BlueprintImplementableEvent) void OnHoverEnd();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void LockVisualBP(const bool bLockedVal);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable) void SetMeshBP(UStaticMesh* NewMesh);

public:
	FBreachOwnershipChanged BreachOwnershipChanged;
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FBuildingBoolDelegate BuildingAddedOnBreach;
	

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FOwner OwnerWithTeam;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) ABuilding* BuildingOnBreach = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bBuildingOnBreach = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector BuildingSpawnOffset = FVector(0, 0, 100);
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bLocked = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bIsRealBreach = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RepulsorRange = 400;
	
	UPROPERTY(BlueprintReadOnly) bool bHovered = false;

	bool UIisOpened = false;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breach") AMainBuilding* MainBuilding = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug") bool bDebug = false;

	bool bUseForgOfWar;
	TSubclassOf<AFogOfWarManager> FogOfWarManagerClass;
};
