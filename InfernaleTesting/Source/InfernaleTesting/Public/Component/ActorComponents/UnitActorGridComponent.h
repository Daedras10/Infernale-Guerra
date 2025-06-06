// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitActorGridComponent.generated.h"


class IFluxRepulsor;
enum class EPlayerOwning : uint8;
class ALDElement;
class ABuildingParent;
class AUnitActor;
class AUnitActorManager;

USTRUCT()
struct FUnitActorGridCell
{
	GENERATED_BODY()

public:
	FIntVector2 GridPosition;
	TArray<TWeakObjectPtr<AUnitActor>> UnitActors = TArray<TWeakObjectPtr<AUnitActor>>();
	TArray<TWeakObjectPtr<ABuildingParent>> Buildings = TArray<TWeakObjectPtr<ABuildingParent>>();
	TArray<TWeakObjectPtr<ALDElement>> LDElements = TArray<TWeakObjectPtr<ALDElement>>();
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UUnitActorGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUnitActorGridComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddUnitActor(TWeakObjectPtr<AUnitActor> UnitActor);
	void RemoveUnitActor(TWeakObjectPtr<AUnitActor> UnitActor);

	void AddBuilding(TWeakObjectPtr<ABuildingParent> Building);
	void RemoveBuilding(TWeakObjectPtr<ABuildingParent> Building);

	void CheckUnitRange(TWeakObjectPtr<AUnitActor> UnitActor);
	FUnitActorGridCell GetAllInRange(FVector Location, float Range, float Angle, FVector Forward, bool bDebugVal);
	bool WasGridCreated() const;

	void AddLDElement(TWeakObjectPtr<ALDElement> LDElement);
	void RemoveLDElement(TWeakObjectPtr<ALDElement> LDElement);

	void GetUnitsOfPlayer(TArray<TWeakObjectPtr<AUnitActor>>& Units, EPlayerOwning PlayerOwning);
	void GetBuildingsOfPlayer(TArray<TWeakObjectPtr<ABuildingParent>>& Buildings, EPlayerOwning PlayerOwning);
	
	TArray<IFluxRepulsor*> GetRepulsorsInRange(FVector Location, float Range);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void CreateGrid();

	FIntVector2 GetGridPositionFromWorldPosition(FVector WorldPosition);
	FIntVector2 GetGridPositionFromWorldPositionWithMinMax(FVector WorldPosition);
	FUnitActorGridCell* GetGridCellFromWorldPosition(FIntVector2 GridPosition);
	int GetGridIndexFromGrid(FIntVector2 GridPosition);
	int GetGridIndexFromWorldPosition(FVector WorldPosition);

	UFUNCTION(CallInEditor)	void DebugGridVisual() const;
	UFUNCTION(CallInEditor)	void DebugGridCenter() const;

	UFUNCTION() void OnUnitActorMoved(AUnitActor* UnitActor, FVector OldLocation);
	UFUNCTION() void OnUnitActorDestroyed(AUnitActor* UnitActor);
	UFUNCTION() void OnBuildingParentDestroyed(ABuildingParent* Building);


	
protected:
	AUnitActorManager* UnitActorManager;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D GridSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D GridCellSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector GridInitialPosition = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsGridCentered = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bDebug = true;

	TArray<FUnitActorGridCell> GridCells;
	bool GridCreated = false;
	bool bDebugTargets = false;
	FUnitActorGridCell CreationCell = FUnitActorGridCell();
	
};