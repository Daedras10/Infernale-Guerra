// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/PathfindingGridComponent.h"

#include "FunctionLibraries/FunctionLibraryInfernale.h"
#include "Manager/UnitActorManager.h"

FPathfindingGridCell::FPathfindingGridCell(): GridPosition(), GridPosition2D(FVector2D(0, 0))
{
}

// Sets default values for this component's properties
UPathfindingGridComponent::UPathfindingGridComponent(): UnitActorManager(nullptr)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPathfindingGridComponent::BeginPlay()
{
	Super::BeginPlay();
	UnitActorManager = Cast<AUnitActorManager>(GetOwner());

	// ...
	
}

FIntVector2 UPathfindingGridComponent::GetGridPositionFromWorldPosition(FVector WorldPosition)
{
	const auto GridStartX = bIsGridCentered ? GridInitialPosition.X - GridSize.X * 0.5 * GridCellSize.X : GridInitialPosition.X;
	const auto GridStartY = bIsGridCentered ? GridInitialPosition.Y - GridSize.Y * 0.5 * GridCellSize.Y : GridInitialPosition.Y;

	if (WorldPosition.X < GridStartX || WorldPosition.Y < GridStartY) return FIntVector2(-1, -1);
	if (WorldPosition.X > GridStartX + GridSize.X * GridCellSize.X || WorldPosition.Y > GridStartY + GridSize.Y * GridCellSize.Y) return FIntVector2(-1, -1);

	if (GridCellSize.X == 0 || GridCellSize.Y == 0) return FIntVector2(-1, -1);
	if (GridSize.X == 0 || GridSize.Y == 0) return FIntVector2(-1, -1);

	const auto X = (WorldPosition.X - GridStartX) / GridCellSize.X;
	const auto Y = (WorldPosition.Y - GridStartY) / GridCellSize.Y;

	return FIntVector2(FMath::Floor(X), FMath::Floor(Y));
}

FIntVector2 UPathfindingGridComponent::GetGridPositionFromWorldPositionWithMinMax(FVector WorldPosition)
{
	const auto GridStartX = bIsGridCentered ? GridInitialPosition.X - GridSize.X * 0.5 * GridCellSize.X : GridInitialPosition.X;
	const auto GridStartY = bIsGridCentered ? GridInitialPosition.Y - GridSize.Y * 0.5 * GridCellSize.Y : GridInitialPosition.Y;

	const auto X = (WorldPosition.X - GridStartX) / GridCellSize.X;
	const auto Y = (WorldPosition.Y - GridStartY) / GridCellSize.Y;

	auto GridPosition = FIntVector2(FMath::Floor(X), FMath::Floor(Y));
	GridPosition.X = FMath::Clamp(GridPosition.X, 0, GridSize.X - 1);
	GridPosition.Y = FMath::Clamp(GridPosition.Y, 0, GridSize.Y - 1);

	return GridPosition;
}

int UPathfindingGridComponent::GetGridIndexFromGrid(FIntVector2 GridPosition)
{
	return GridPosition.X * GridSize.Y + GridPosition.Y;
}

int UPathfindingGridComponent::GetGridIndexFromWorldPosition(FVector WorldPosition)
{
	const auto GridPosition = GetGridPositionFromWorldPosition(WorldPosition);
	return GetGridIndexFromGrid(GridPosition);
}

FPathfindingGridCell* UPathfindingGridComponent::GetGridCellFromWorldPosition(FIntVector2 GridPosition)
{
	return &GridCells[GridPosition.X * GridSize.Y + GridPosition.Y];
}

void UPathfindingGridComponent::CreateGrid()
{
	ClearGrid();
	for (int i = 0; i < GridSize.Y; i++)
	{
		for (int j = 0; j < GridSize.X; j++)
		{
			FPathfindingGridCell PathfindingGridCell = FPathfindingGridCell();
			PathfindingGridCell.GridPosition = FIntVector2(j, i);
			PathfindingGridCell.GridPosition2D = FVector2D(j, i);
			GridCells.Add(PathfindingGridCell);
		}
	}
	GridCreated = true;
}

void UPathfindingGridComponent::ClearGrid()
{
	GridCells.Empty();
	GridCreated = false;
}

void UPathfindingGridComponent::UpdateCollision()
{
	if (!GridCreated) return;
	
	const auto InitialX = bIsGridCentered ? GridInitialPosition.X - GridSize.X * 0.5 * GridCellSize.X : GridInitialPosition.X;
	const auto InitialY = bIsGridCentered ? GridInitialPosition.Y - GridSize.Y * 0.5 * GridCellSize.Y : GridInitialPosition.Y;

	for (int i = 0; i < GridSize.X; i++)
	{
		for (int j = 0; j < GridSize.Y; j++)
		{
			auto MiddleX = InitialX + i * GridCellSize.X + GridCellSize.X * 0.5;
			auto MiddleY = InitialY + j * GridCellSize.Y + GridCellSize.Y * 0.5;

			auto HitResult = FHitResult();
			const auto Start = FVector(MiddleX, MiddleY, GridDrawZ);
			const auto End = Start + FVector(0, 0, 100000);
			FCollisionQueryParams CollisionParams;
			auto Channel = UFunctionLibraryInfernale::GetCustomTraceChannel(ECustomTraceChannel::PathfindingCollision);
			GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, Channel, CollisionParams);
			GridCells[i * GridSize.Y + j].bIsObstacle = HitResult.bBlockingHit;
			
			auto Color = GridCells[i * GridSize.Y + j].bIsObstacle ? FColor::Red : FColor::Green;
			DrawDebugLine(GetWorld(), Start, End, Color, false, RefreshRate, 0, 100);
		}
	}
}

void UPathfindingGridComponent::GridVisualTool()
{
	if (!bShowGridVisualTool) return;
	if (!GridCreated) return;
	if (GridCells.Num() == 0)
	{
		CreateGrid();
		UpdateCollision();
	}
	
	const auto InitialX = bIsGridCentered ? GridInitialPosition.X - GridSize.X * 0.5 * GridCellSize.X : GridInitialPosition.X;
	const auto InitialY = bIsGridCentered ? GridInitialPosition.Y - GridSize.Y * 0.5 * GridCellSize.Y : GridInitialPosition.Y;

	for (int i = 0; i < GridSize.X; i++)
	{
		for (int j = 0; j < GridSize.Y; j++)
		{
			auto StartLine = FVector(InitialX + i * GridCellSize.X, InitialY + j * GridCellSize.Y, GridDrawZ);
			auto EndLine = FVector(InitialX + (i + 1) * GridCellSize.X, InitialY + j * GridCellSize.Y, GridDrawZ);
			DrawDebugLine(GetWorld(),StartLine, EndLine, GridColor, false, RefreshRate, 0, 100);

			auto MiddleX = StartLine.X + GridCellSize.X * 0.5;
			auto MiddleY = StartLine.Y + GridCellSize.Y * 0.5;

			auto Color = GridCells[i * GridSize.Y + j].bIsObstacle ? FColor::Red : FColor::Green;
			DrawDebugBox(GetWorld(), FVector(MiddleX, MiddleY, GridDrawZ), FVector(GridCellSize.X * 0.25, GridCellSize.Y * 0.25, 0), Color, false, RefreshRate, 0, 100);
		}
		auto StartLine = FVector(InitialX + i * GridCellSize.X, InitialY, GridDrawZ);
		auto EndLine = FVector(InitialX + i * GridCellSize.X, InitialY + GridSize.Y * GridCellSize.Y, GridDrawZ);
		DrawDebugLine(GetWorld(), StartLine, EndLine, GridColor, false, RefreshRate, 0, 100);
	}
	DrawDebugLine(GetWorld(), FVector(InitialX + (GridSize.X) * GridCellSize.X, InitialY, GridDrawZ), FVector(InitialX + (GridSize.X) * GridCellSize.X, InitialY + GridSize.Y * GridCellSize.Y, GridDrawZ), GridColor, false, RefreshRate, 0, 100);
	DrawDebugLine(GetWorld(), FVector(InitialX, InitialY + GridSize.Y * GridCellSize.Y, GridDrawZ), FVector(InitialX + GridSize.X * GridCellSize.X, InitialY + GridSize.Y * GridCellSize.Y, GridDrawZ), GridColor, false, RefreshRate, 0, 100);
}

void UPathfindingGridComponent::GridCenterTool()
{
	if (!bShowGridCenterTool) return;
	DrawDebugSphere(GetWorld(), GridInitialPosition, 1000, 12, GridColor, false, RefreshRate, 0, 100);
}

void UPathfindingGridComponent::GridbUpdateCollisionTool()
{
	if (!bUpdateCollisionTool) return;
	UpdateCollision();
}


// Called every frame
void UPathfindingGridComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UPathfindingGridComponent::BlueprintEditorTick(float DeltaTime)
{
	if (!GridCreated) return;
	if (!ShouldEditorTick) return;

	if (EditorTickElapsed >= RefreshRate * 0.9f)
	{
		GridVisualTool();
		GridCenterTool();
		GridbUpdateCollisionTool();
		EditorTickElapsed = 0;
		return;
	}
	EditorTickElapsed += DeltaTime;
}

