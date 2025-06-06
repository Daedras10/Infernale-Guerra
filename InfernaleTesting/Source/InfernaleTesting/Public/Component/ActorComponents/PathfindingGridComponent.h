// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PathfindingGridComponent.generated.h"


class AUnitActorManager;


USTRUCT(Blueprintable)
struct FPathfindingGridCell
{
	GENERATED_BODY()
public:
	FPathfindingGridCell();

public:
	FIntVector2 GridPosition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FVector2D GridPosition2D;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bIsObstacle = false;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INFERNALETESTING_API UPathfindingGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPathfindingGridComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void BlueprintEditorTick(float DeltaTime);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	FIntVector2 GetGridPositionFromWorldPosition(FVector WorldPosition);
	FIntVector2 GetGridPositionFromWorldPositionWithMinMax(FVector WorldPosition);
	int GetGridIndexFromGrid(FIntVector2 GridPosition);
	int GetGridIndexFromWorldPosition(FVector WorldPosition);
	FPathfindingGridCell* GetGridCellFromWorldPosition(FIntVector2 GridPosition);
	
	UFUNCTION(CallInEditor) void CreateGrid();
	UFUNCTION(CallInEditor) void ClearGrid();
	UFUNCTION(CallInEditor) void UpdateCollision();

	UFUNCTION()	void GridVisualTool();
	UFUNCTION()	void GridCenterTool();
	UFUNCTION()	void GridbUpdateCollisionTool();

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool ShouldEditorTick = false;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FPathfindingGridCell> GridCells;
	
	const float EditorTickDuration = 0.0167f;
	float EditorTickElapsed = 10.0f;
	float RefreshRate = 1.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere) float GridDrawZ = 100.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bShowGridCenterTool = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bShowGridVisualTool = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool bUpdateCollisionTool = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) bool GridCreated = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) FColor GridColor = FColor::Black;
	
	FPathfindingGridCell CreationCell = FPathfindingGridCell();
};
