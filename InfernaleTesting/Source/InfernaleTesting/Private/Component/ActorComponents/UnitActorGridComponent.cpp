// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/ActorComponents/UnitActorGridComponent.h"

#include "LD/Buildings/BuildingParent.h"
#include "LD/Buildings/MainBuilding.h"
#include "LD/LDElement/LDElement.h"
#include "LD/Breach.h"
#include "Manager/UnitActorManager.h"
#include "UnitAsActor/UnitActor.h"

// Sets default values for this component's properties
UUnitActorGridComponent::UUnitActorGridComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UUnitActorGridComponent::BeginPlay()
{
	Super::BeginPlay();
	UnitActorManager = Cast<AUnitActorManager>(GetOwner());
	CreateGrid();
	// ...
	
}

void UUnitActorGridComponent::CreateGrid()
{
	GridCells.Empty();
	for (int i = 0; i < GridSize.Y; i++)
	{
		for (int j = 0; j < GridSize.X; j++)
		{
			FUnitActorGridCell UnitActorGridCell = FUnitActorGridCell();
			UnitActorGridCell.GridPosition = FIntVector2(j, i);
			GridCells.Add(UnitActorGridCell);
		}
	}
	GridCreated = true;
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Grid Created"));

	for (auto Building : CreationCell.Buildings)
	{
		AddBuilding(Building);
	}
	for (auto UnitActor : CreationCell.UnitActors)
	{
		AddUnitActor(UnitActor);
	}
	for (auto LDElement : CreationCell.LDElements)
	{
		AddLDElement(LDElement);
	}
	
	CreationCell.Buildings.Empty();
	CreationCell.UnitActors.Empty();
	CreationCell.LDElements.Empty();
}

FIntVector2 UUnitActorGridComponent::GetGridPositionFromWorldPosition(FVector WorldPosition)
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

FIntVector2 UUnitActorGridComponent::GetGridPositionFromWorldPositionWithMinMax(FVector WorldPosition)
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

FUnitActorGridCell* UUnitActorGridComponent::GetGridCellFromWorldPosition(FIntVector2 GridPosition)
{
	return &GridCells[GridPosition.X * GridSize.Y + GridPosition.Y];
}

int UUnitActorGridComponent::GetGridIndexFromGrid(FIntVector2 GridPosition)
{
	return GridPosition.X * GridSize.Y + GridPosition.Y;
}

int UUnitActorGridComponent::GetGridIndexFromWorldPosition(FVector WorldPosition)
{
	const auto GridPosition = GetGridPositionFromWorldPosition(WorldPosition);
	return GetGridIndexFromGrid(GridPosition);
}

void UUnitActorGridComponent::DebugGridVisual() const
{
	const auto InitialX = bIsGridCentered ? GridInitialPosition.X - GridSize.X * 0.5 * GridCellSize.X : GridInitialPosition.X;
	const auto InitialY = bIsGridCentered ? GridInitialPosition.Y - GridSize.Y * 0.5 * GridCellSize.Y : GridInitialPosition.Y;

	for (int i = 0; i < GridSize.X; i++)
	{
		for (int j = 0; j < GridSize.Y; j++)
		{
			auto StartLine = FVector(InitialX + i * GridCellSize.X, InitialY + j * GridCellSize.Y, GridInitialPosition.Z);
			auto EndLine = FVector(InitialX + (i + 1) * GridCellSize.X, InitialY + j * GridCellSize.Y, GridInitialPosition.Z);
			DrawDebugLine(GetWorld(),StartLine, EndLine, FColor::Red, false, 10, 0, 100);
		}
		auto StartLine = FVector(InitialX + i * GridCellSize.X, InitialY, GridInitialPosition.Z);
		auto EndLine = FVector(InitialX + i * GridCellSize.X, InitialY + GridSize.Y * GridCellSize.Y, GridInitialPosition.Z);
		DrawDebugLine(GetWorld(), StartLine, EndLine, FColor::Red, false, 10, 0, 100);
	}
}

void UUnitActorGridComponent::DebugGridCenter() const
{
	DrawDebugSphere(GetWorld(), GridInitialPosition, 1000, 12, FColor::Red, false, 10, 0, 100);
}

void UUnitActorGridComponent::OnUnitActorMoved(AUnitActor* UnitActor, FVector OldLocation)
{
	const TWeakObjectPtr<AUnitActor> UnitActorPtr = TWeakObjectPtr<AUnitActor>(UnitActor);
	if (!UnitActorPtr.IsValid()) return;
	auto GridCellIndexInitialPos = GetGridIndexFromWorldPosition(OldLocation);
	auto GridCellIndexNewPos = GetGridIndexFromWorldPosition(UnitActorPtr->GetActorLocation());

	if (GridCellIndexInitialPos == GridCellIndexNewPos) return;
	if (GridCellIndexInitialPos < 0 || GridCellIndexInitialPos >= GridCells.Num()) return;
	if (GridCellIndexNewPos < 0 || GridCellIndexNewPos >= GridCells.Num()) return;

	GridCells[GridCellIndexInitialPos].UnitActors.Remove(UnitActorPtr);
	GridCells[GridCellIndexNewPos].UnitActors.Add(UnitActorPtr);
}

void UUnitActorGridComponent::OnUnitActorDestroyed(AUnitActor* UnitActorObj)
{
	const TWeakObjectPtr<AUnitActor> UnitActor = TWeakObjectPtr<AUnitActor>(UnitActorObj);
	if (!UnitActor.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("UUnitActorGridComponent::OnUnitActorDestroyed UnitActor is not valid"));
		return;
	}
	const auto GridCellIndex = GetGridIndexFromWorldPosition(UnitActor->GetActorLocation());
	GridCells[GridCellIndex].UnitActors.Remove(UnitActor);
}

void UUnitActorGridComponent::OnBuildingParentDestroyed(ABuildingParent* BuildingObj)
{
	const TWeakObjectPtr<ABuildingParent> Building = TWeakObjectPtr<ABuildingParent>(BuildingObj);
	if (!Building.IsValid()) return;
	const auto GridCellIndex = GetGridIndexFromWorldPosition(Building->GetActorLocation());
	GridCells[GridCellIndex].Buildings.Remove(Building);
}


// Called every frame
void UUnitActorGridComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UUnitActorGridComponent::AddUnitActor(TWeakObjectPtr<AUnitActor> UnitActor)
{
	if (!GridCreated)
	{
		CreationCell.UnitActors.Add(UnitActor);
		return;
	}
	if (!UnitActor.IsValid()) return;
	const auto GridCellIndex = GetGridIndexFromWorldPosition(UnitActor->GetActorLocation());
	GridCells[GridCellIndex].UnitActors.Add(UnitActor);

	UnitActor->UnitActorMoved.AddDynamic(this, &UUnitActorGridComponent::OnUnitActorMoved);
	UnitActor->UnitActorDestroyed.AddDynamic(this, &UUnitActorGridComponent::OnUnitActorDestroyed);

	auto UnitsByPlayers = UnitActorManager->GetUnitActorsByPlayers();
	auto Player = UnitActor->GetOwner().Player;
	UnitsByPlayers[Player].UnitActors.Add(UnitActor);
}

void UUnitActorGridComponent::RemoveUnitActor(TWeakObjectPtr<AUnitActor> UnitActor)
{
	OnUnitActorDestroyed(UnitActor.Get());
	auto UnitsByPlayers = UnitActorManager->GetUnitActorsByPlayers();
	const auto Player = UnitActor->GetOwner().Player;
	UnitsByPlayers[Player].UnitActors.Remove(UnitActor);
}

void UUnitActorGridComponent::AddBuilding(TWeakObjectPtr<ABuildingParent> Building)
{
	if (!GridCreated)
	{
		CreationCell.Buildings.Add(Building);
		return;
	}
	if (!Building.IsValid()) return;
	const auto GridCellIndex = GetGridIndexFromWorldPosition(Building->GetActorLocation());
	GridCells[GridCellIndex].Buildings.Add(Building);

	Building->BuildingParentDestroyed.AddDynamic(this, &UUnitActorGridComponent::OnBuildingParentDestroyed);
}

void UUnitActorGridComponent::RemoveBuilding(TWeakObjectPtr<ABuildingParent> Building)
{
	OnBuildingParentDestroyed(Building.Get());
}

void UUnitActorGridComponent::CheckUnitRange(TWeakObjectPtr<AUnitActor> UnitActor)
{
	if (!UnitActor.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("UUnitActorGridComponent::CheckUnitRange UnitActor is not valid"));
		return;
	}
	
	auto UnitActorStruct = UnitActor->GetUnitStruct();
	auto Range = UnitActorStruct.BaseDetectionRange;
	auto Angle = UnitActorStruct.BaseDetectionAngle;
	auto Center = UnitActor->GetActorLocation();

	auto UnitsInRange = TArray<TWeakObjectPtr<AUnitActor>>();
	auto BuildingsInRange = TArray<TWeakObjectPtr<ABuildingParent>>();
	auto LDElementsInRange = TArray<TWeakObjectPtr<ALDElement>>();

	auto GridPosition = GetGridPositionFromWorldPosition(Center);

	auto WorldMin = Center - FVector(Range, Range, 0);
	auto WorldMax = Center + FVector(Range, Range, 0);

	auto CheckMin = GetGridPositionFromWorldPositionWithMinMax(WorldMin);
	auto CheckMax = GetGridPositionFromWorldPositionWithMinMax(WorldMax);

	// Get all units and buildings in range
	for (int i = CheckMin.X; i <= CheckMax.X; i++)
	{
		for (int j = CheckMin.Y; j <= CheckMax.Y; j++)
		{
			auto GridCellIndex = GetGridIndexFromGrid(FIntVector2(i, j));
			if (GridCellIndex < 0 || GridCellIndex >= GridCells.Num()) continue;

			for (auto Unit : GridCells[GridCellIndex].UnitActors)
			{
				if (!Unit.IsValid()) continue;
				if (Unit.Get() == UnitActor.Get()) continue;
				
				auto Distance = FVector::Dist(Center, Unit->GetActorLocation());
				if (Distance <= Range)
				{
					auto Direction = (Unit->GetActorLocation() - Center).GetSafeNormal();
					auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, UnitActor->GetActorForwardVector())));
					if (AngleBetween <= Angle) UnitsInRange.Add(Unit);
				}
			}

			for (auto Building : GridCells[GridCellIndex].Buildings)
			{
				if (!Building.IsValid()) continue;
				auto Distance = FVector::Dist(Center, Building->GetActorLocation());
				if (Distance <= Range)
				{
					auto Direction = (Building->GetActorLocation() - Center).GetSafeNormal();
					auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, UnitActor->GetActorForwardVector())));
					if (AngleBetween <= Angle)
					{
						BuildingsInRange.Add(Building);
					}
				}
			}

			for (auto LDElement : GridCells[GridCellIndex].LDElements)
			{
				if (!LDElement.IsValid()) continue;
				auto Distance = FVector::Dist(Center, LDElement->GetActorLocation());
				if (Distance <= Range)
				{
					auto Direction = (LDElement->GetActorLocation() - Center).GetSafeNormal();
					auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, UnitActor->GetActorForwardVector())));
					if (AngleBetween <= Angle)
					{
						LDElementsInRange.Add(LDElement);
					}
				}
			}
		}
	}

	TWeakObjectPtr<AActor> ClosestActor = nullptr;
	auto ClosestDistance = TNumericLimits<float>::Max();
	EUnitTargetType TargetType = EUnitTargetType::UTargetNone;

	// Check for Enemies
	for (auto Unit : UnitsInRange)
	{
		if (Unit->GetOwner().Team != UnitActor->GetOwner().Team)
		{
			if (!Unit.IsValid()) continue;
			if (Unit.Get() == UnitActor.Get()) continue;
			auto UnitCanHaveAdditionalAttackers = Unit->CanHaveAdditionalAttackers();
			auto UnitIsAlreadyAttacker = Unit->IsAttacker(UnitActor);
			if (!UnitIsAlreadyAttacker) if (!UnitCanHaveAdditionalAttackers) continue;
			const auto Distance = FVector::Dist(Center, Unit->GetActorLocation());
			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestActor = Unit;
				TargetType = EUnitTargetType::UTargetUnit;
			}
			//DrawDebugLine(GetWorld(), Center, Unit->GetActorLocation(), FColor::Blue, false, .3f, 0, 100);
			continue;
		}
		//DrawDebugLine(GetWorld(), Center, Unit->GetActorLocation(), FColor::Red, false, 1, 0, 100);
	}

	for (auto Building : BuildingsInRange)
	{
		if (Building->GetOwner().Team != UnitActor->GetOwner().Team)
		{
			const auto Distance = FVector::Dist(Center, Building->GetActorLocation());
			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestActor = Building;
				TargetType = EUnitTargetType::UTargetBuilding;
			}
			//DrawDebugLine(GetWorld(), Center, Building->GetActorLocation(), FColor::Cyan, false, 1, 0, 100);
			continue;
		}
		//DrawDebugLine(GetWorld(), Center, Building->GetActorLocation(), FColor::Purple, false, 1, 0, 100);
	}

	/* Check for Neutral Camps */
	for (auto LDElement : LDElementsInRange)
	{
		const auto Distance = FVector::Dist(Center, LDElement->GetActorLocation());

		// Remove later
		if (LDElement->GetLDElementType() != ELDElementType::LDElementNeutralCampType) continue;
		
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = LDElement;
			TargetType = EUnitTargetType::UTargetNeutralCamp;
		}
		//DrawDebugLine(GetWorld(), Center, LDElement->GetActorLocation(), FColor::Green, false, 1, 0, 100);
	}
	
	if (ClosestActor == nullptr)
	{
		if (UnitActor->GetTarget() != nullptr)
		{
			UnitActor->SetTargetToReplicate(ClosestActor, EUnitTargetType::UTargetNone);
			UnitActor->AddUnitForPathfindingRefresh();
		}
		return;
	}

	//UnitActorManager->DrawDebugLineMulticast(Center, ClosestActor->GetActorLocation(), FColor::Red, .5f, 100);
	if (bDebugTargets) DrawDebugLine(GetWorld(), Center, ClosestActor->GetActorLocation(), FColor::Red, false, .5f, 0, 100);
	UnitActor->SetTargetToReplicate(ClosestActor, TargetType);
}

FUnitActorGridCell UUnitActorGridComponent::GetAllInRange(FVector Center, float Range, float Angle, FVector Forward, bool bDebugVal)
{
	FUnitActorGridCell GridCell = FUnitActorGridCell();

	auto UnitsInRange = TArray<TWeakObjectPtr<AUnitActor>>();
	auto BuildingsInRange = TArray<TWeakObjectPtr<ABuildingParent>>();
	auto LDElementsInRange = TArray<TWeakObjectPtr<ALDElement>>();

	auto GridPosition = GetGridPositionFromWorldPosition(Center);

	auto WorldMin = Center - FVector(Range, Range, 0);
	auto WorldMax = Center + FVector(Range, Range, 0);

	auto CheckMin = GetGridPositionFromWorldPositionWithMinMax(WorldMin);
	auto CheckMax = GetGridPositionFromWorldPositionWithMinMax(WorldMax);

	if (bDebugVal) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Check : aa")));
				

	// Get all units and buildings in range
	for (int i = CheckMin.X; i <= CheckMax.X; i++)
	{
		for (int j = CheckMin.Y; j <= CheckMax.Y; j++)
		{
			auto GridCellIndex = GetGridIndexFromGrid(FIntVector2(i, j));
			if (GridCellIndex < 0 || GridCellIndex >= GridCells.Num()) continue;

			if (bDebugVal) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Checkinbg GridCellIndex : %d, units: %d"), GridCellIndex, GridCells[GridCellIndex].UnitActors.Num()));

			for (auto Unit : GridCells[GridCellIndex].UnitActors)
			{
				if (bDebugVal) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Unit : aa")));
				if (!Unit.IsValid()) continue;
				if (Unit == nullptr) continue;
				auto Distance = FVector::Dist(Center, Unit->GetActorLocation());
				if (Distance <= Range)
				{
					auto Direction = (Unit->GetActorLocation() - Center).GetSafeNormal();
					auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, Forward)));
					if (AngleBetween <= Angle) UnitsInRange.Add(Unit);
					// else
					// {
					// 	//Engine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Angle Between : %f"), AngleBetween));
					// }
				}
			}

			for (auto Building : GridCells[GridCellIndex].Buildings)
			{
				if (!Building.IsValid()) continue;
				auto Distance = FVector::Dist(Center, Building->GetActorLocation());
				if (Distance <= Range)
				{
					auto Direction = (Building->GetActorLocation() - Center).GetSafeNormal();
					auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, Forward)));
					if (AngleBetween <= Angle)
					{
						BuildingsInRange.Add(Building);
					}
				}
			}

			for (auto LDElement : GridCells[GridCellIndex].LDElements)
			{
				if (!LDElement.IsValid()) continue;
				auto Distance = FVector::Dist(Center, LDElement->GetActorLocation());
				if (Distance <= Range)
				{
					auto Direction = (LDElement->GetActorLocation() - Center).GetSafeNormal();
					auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, Forward)));
					if (AngleBetween <= Angle)
					{
						LDElementsInRange.Add(LDElement);
					}
				}
			}
		}
	}

	GridCell.UnitActors = UnitsInRange;
	GridCell.Buildings = BuildingsInRange;
	GridCell.LDElements = LDElementsInRange;
	return GridCell;
}

bool UUnitActorGridComponent::WasGridCreated() const
{
	return GridCreated;
}

void UUnitActorGridComponent::AddLDElement(TWeakObjectPtr<ALDElement> LDElement)
{
	if (!GridCreated)
	{
		CreationCell.LDElements.Add(LDElement);
		return;
	}
	if (!LDElement.IsValid()) return;
	const auto GridCellIndex = GetGridIndexFromWorldPosition(LDElement->GetActorLocation());
	GridCells[GridCellIndex].LDElements.Add(LDElement);
}

void UUnitActorGridComponent::RemoveLDElement(TWeakObjectPtr<ALDElement> LDElement)
{
	const auto GridCellIndex = GetGridIndexFromWorldPosition(LDElement->GetActorLocation());
	GridCells[GridCellIndex].LDElements.Remove(LDElement);
}

void UUnitActorGridComponent::GetUnitsOfPlayer(TArray<TWeakObjectPtr<AUnitActor>>& Units, EPlayerOwning PlayerOwning)
{
	for (auto GridCell : GridCells)
	{
		for (auto Unit : GridCell.UnitActors)
		{
			if (Unit->GetOwner().Player == PlayerOwning) Units.Add(Unit);
		}
	}
}

void UUnitActorGridComponent::GetBuildingsOfPlayer(TArray<TWeakObjectPtr<ABuildingParent>>& Buildings, EPlayerOwning PlayerOwning)
{
	for (auto GridCell : GridCells)
	{
		for (auto Building : GridCell.Buildings)
		{
			if (Building->GetOwner().Player == PlayerOwning) Buildings.Add(Building);
		}
	}
}

TArray<IFluxRepulsor*> UUnitActorGridComponent::GetRepulsorsInRange(FVector Center, float Range)
{
	FUnitActorGridCell GridCell = FUnitActorGridCell();

	auto RepulsorsInRange = TArray<IFluxRepulsor*>();
	auto BuildingsInRange = TArray<TWeakObjectPtr<ABuildingParent>>();

	auto WorldMin = Center - FVector(Range, Range, 0);
	auto WorldMax = Center + FVector(Range, Range, 0);

	auto CheckMin = GetGridPositionFromWorldPositionWithMinMax(WorldMin);
	auto CheckMax = GetGridPositionFromWorldPositionWithMinMax(WorldMax);

	// Get all units and buildings in range
	for (int i = CheckMin.X; i <= CheckMax.X; i++)
	{
		for (int j = CheckMin.Y; j <= CheckMax.Y; j++)
		{
			auto GridCellIndex = GetGridIndexFromGrid(FIntVector2(i, j));

			for (auto Building : GridCells[GridCellIndex].Buildings)
			{
				if (!Building.IsValid()) continue;
				const auto Distance = FVector::Dist(Center, Building->GetActorLocation());
				if (Distance > Range) continue;
				BuildingsInRange.Add(Building);
				RepulsorsInRange.Add(Cast<IFluxRepulsor>(Building));
				const auto MainBuilding = Cast<AMainBuilding>(Building);
				if (!MainBuilding) continue;
				for (ABreach* Breach : MainBuilding->GetBreaches())
				{
					IFluxRepulsor* Repulsor = Cast<IFluxRepulsor>(Breach);
					RepulsorsInRange.Add(Repulsor);
				}
			}

			// for (auto LDElement : GridCells[GridCellIndex].LDElements)
			// {
			// 	if (!LDElement) continue;
			// 	auto Distance = FVector::Dist(Center, LDElement->GetActorLocation());
			// 	if (Distance <= Range)
			// 	{
			// 		auto Direction = (LDElement->GetActorLocation() - Center).GetSafeNormal();
			// 		auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, Forward)));
			// 		if (AngleBetween <= Angle)
			// 		{
			// 			LDElementsInRange.Add(LDElement);
			// 		}
			// 	}
			// }
		}
	}
	return RepulsorsInRange;
}

