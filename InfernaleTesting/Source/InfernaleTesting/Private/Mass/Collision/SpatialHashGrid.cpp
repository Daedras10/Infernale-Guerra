// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Collision/SpatialHashGrid.h"
#include <LD/LDElement/SoulBeacon.h>
#include <LD/LDElement/NeutralCamp.h>
#include <LD/LDElement/Boss.h>
#include "LeData/DataGathererActor.h"

ASpatialHashGrid* ASpatialHashGrid::Instance;

// Sets default values
ASpatialHashGrid::ASpatialHashGrid()
{
	Instance = this;

	PrimaryActorTick.bCanEverTick = false;
}

ASpatialHashGrid::ASpatialHashGrid(bool IsPivotCentered) : bIsPivotCentered(IsPivotCentered)
{
	Instance = this;
	PrimaryActorTick.bCanEverTick = false;
}

/*
* Adds the Mass Handle of an entity to the grid and references its data.
*
* @param WorldCoordinates : World Location of the entity
* @param Entity : Entity added to the grid
* @param Owner : The entity owner
* @param Transform : A pointer to the entity's transform, so that it can be referenced
* @param Health : The entity's health at the time of addition
* @return False if the adding conditions aren't met, True otherwise.
*/
bool ASpatialHashGrid::AddEntityToGrid(FVector WorldCoordinates, FMassEntityHandle Entity, FOwner Owner,
                                       FTransformFragment* Transform, float Health, float TargetableRadius,
                                       EEntityType Type)
{
	FIntVector2 GridCoordinates = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoordinates), TEXT("SpatialHashGrid Error : Coordinates out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoordinates), "SpatialHashGrid Error : Coordinates out of grid",
	                    Instance->bUseAsserts)) return false;

	if (Contains(Entity)) return false;

	int CoordsIndex = CoordsToIndex(GridCoordinates);
	Instance->GridCells[CoordsIndex].Entities.Add(
		Entity, GridCellEntityData(Owner, Transform->GetTransform().GetLocation(), Health, TargetableRadius, Type));
	Instance->HandleToCoordsMap.Add(Entity, GridCoordinates);

	Instance->GridCells[CoordsIndex].UpdatePresentOwners();

	Instance->RefreshPresent();

	return true;
}

/*
* @param Entity : Entity removed from the cell
* 
* @return False if the removing conditions aren't met, True otherwise.
*/
bool ASpatialHashGrid::RemoveEntityFromGrid(FMassEntityHandle Entity)
{
	//checkf(Contains(Entity), TEXT("SpatialHashGrid Error : Trying to remove unknown entity"));

	if (!DebugCheckExpr(Contains(Entity), "SpatialHashGrid Error : Trying to remove unknown entity",
	                    Instance->bUseAsserts)) return false;

	FIntVector2 CellCoordinates = CoordsFromHandle(Entity);
	if (!IsInGrid(CellCoordinates)) return false;

	int CellIndex = CoordsToIndex(CellCoordinates);
	if (Instance->GridCells[CellIndex].Entities.IsEmpty()) return false;

	Instance->GridCells[CellIndex].Entities.Remove(Entity);
	Instance->HandleToCoordsMap.Remove(Entity);

	Instance->GridCells[CellIndex].UpdatePresentOwners();

	RefreshPresent();

	return true;
}

/*
* @param Entity : Entity to be moved
* @param WorldCoordinates : The Entity's current world coordinates
* 
* @return False if the coordinates are outside the grid, True otherwise
*/

bool ASpatialHashGrid::MoveEntityToCell(FMassEntityHandle Entity, FVector WorldCoordinates)
{
	//checkf(IsInGrid(WorldCoordinates), TEXT("SpatialHashGrid Error : Coordinates out of grid"));

	if (!DebugCheckExpr(IsInGrid(WorldCoordinates), "SpatialHashGrid Error : Coordinates out of grid",
	                    Instance->bUseAsserts)) return false;

	FIntVector2 CurrentCellCoords = CoordsFromHandle(Entity);
	FIntVector2 NewCellCoords = Instance->WorldToGridCoords(WorldCoordinates);

	if (NewCellCoords == CurrentCellCoords) return true;

	GridCellEntityData DataToMove = Instance->GetEntityData(Entity);

	int CurrentCellIndex = CoordsToIndex(CurrentCellCoords);
	int NewCellIndex = CoordsToIndex(NewCellCoords);

	Instance->GridCells[NewCellIndex].Entities.Add(Entity, DataToMove);
	Instance->GridCells[CurrentCellIndex].Entities.Remove(Entity);

	Instance->GridCells[NewCellIndex].UpdatePresentOwners();
	Instance->GridCells[CurrentCellIndex].UpdatePresentOwners();

	Instance->HandleToCoordsMap[Entity] = NewCellCoords;

	if (!Instance->GridCells[NewCellIndex].CellIsValidForUnits)
	{
		return false;
		//DamageEntity(Entity, 99999999.f);
		//TODO should do fall vfx
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid: Entity moved to invalid cell, killing it"));
	}

	return true;
}

void ASpatialHashGrid::UpdateCellTransform(FMassEntityHandle Entity, FTransform Transform)
{
	FIntVector2 CellCoords = CoordsFromHandle(Entity);
	int CellIndex = CoordsToIndex(CellCoords);
	auto GridCell = Instance->GridCells[CellIndex];

	//checkf(GridCell.Entities.Contains(Entity), TEXT("SpatialHashGrid Error : Entity not in cell"));
	if (!DebugCheckExpr(GridCell.Entities.Contains(Entity), "SpatialHashGrid Error : Entity not in cell",
	                    Instance->bUseAsserts)) return;

	Instance->GridCells[CellIndex].Entities[Entity].Location = Transform.GetLocation();
}

void ASpatialHashGrid::DamageEntity(FMassEntityHandle Entity, float Damage)
{
	const auto Data = Instance->GetMutableEntityData(Entity);
	if (Data == nullptr) return;
	Data->DamageEntity(Damage);
}

bool ASpatialHashGrid::AddBuildingToGrid(FVector WorldCoordinates, ABuildingParent* Building)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding building out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Adding building out of grid",
	                    Instance->bUseAsserts)) return false;;

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].Buildings.Contains(Building)) return false;

	// Reference to initial Cell
	Instance->GridCells[CellIndex].Buildings.Add(Building);

	// Reference to cells in range
	TArray<HashGridCell*> CellsInRange = FindCellsInRange(WorldCoordinates, Building->GetTargetableRange(), 360.f,
	                                                      Building->GetActorForwardVector(), FMassEntityHandle(0, 0));
	for (auto Cell : CellsInRange)
		if (!Cell->Buildings.Contains(Building))
			Cell->Buildings.Add(Building);

	return true;
}

bool ASpatialHashGrid::RemoveBuildingFromGrid(FVector WorldCoordinates, ABuildingParent* Building)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Accessing coords out of grid",
	                    Instance->bUseAsserts)) return false;

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].Buildings.IsEmpty()) return false;

	Instance->GridCells[CellIndex].Buildings.Remove(Building);
	return true;
}

bool ASpatialHashGrid::AddLDElementToGrid(FVector WorldCoordinates, ALDElement* LDElement)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding LD Element out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Adding LD Element out of grid",
	                    Instance->bUseAsserts))
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : Adding LD Element out of grid"));
		return false;
	}

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].LDElements.Contains(LDElement))
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : LDElement already in grid"));
		return false;
	}

	Instance->GridCells[CellIndex].LDElements.Add(LDElement);
	// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("LDElement added to grid at %s"), *WorldCoordinates.ToString()));

	TWeakObjectPtr<ABoss> PotentialBoss = Cast<ABoss>(LDElement);
	if (PotentialBoss != nullptr)
	{
		Instance->AllBosses.AddUnique(PotentialBoss);
		TArray<HashGridCell*> Cells = FindCellsInRange(WorldCoordinates, PotentialBoss->GetSummonRange(), 360.f,
		                                               PotentialBoss->GetActorForwardVector(), FMassEntityHandle(0, 0));
		for (auto& Cell : Cells)
		{
			Cell->LinkedBoss = PotentialBoss;
		}
	}

	return true;
}

bool ASpatialHashGrid::RemoveLDElementFromGrid(FVector WorldCoordinates, ALDElement* LDElement)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Accessing coords out of grid",
	                    Instance->bUseAsserts))
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
		return false;
	}
	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].LDElements.IsEmpty())
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : LDElement not in grid"));
		return false;
	}

	Instance->GridCells[CellIndex].LDElements.Remove(LDElement);
	// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("LDElement removed from grid"));
	return true;
}

bool ASpatialHashGrid::AddSoulBeaconToGrid(ASoulBeacon* SoulBeacon)
{
	if (Instance->AllSoulBeacons.Contains(SoulBeacon)) return false;
	const FVector Location = SoulBeacon->GetActorLocation();
	FIntVector2 GridCoords = Instance->WorldToGridCoords(Location);

	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding Soul Beacon out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Adding Soul Beacon out of grid",
	                    Instance->bUseAsserts)) return false;

	int CellIndex = CoordsToIndex(GridCoords);
	Instance->AllSoulBeacons.Add(SoulBeacon);

	TArray<HashGridCell*> CaptureCells = TArray<HashGridCell*>();
	TArray<HashGridCell*> EffectCells = TArray<HashGridCell*>();

	for (const auto& Pt : SoulBeacon->GetCaptureRanges())
		CaptureCells.Append(FindCellsInRange(Pt.GetPosition(), Pt.Radius, 360.f, SoulBeacon->GetActorForwardVector(),
		                                     FMassEntityHandle(0, 0)));

	for (const auto& Pt : SoulBeacon->GetEffectRanges())
		EffectCells.Append(FindCellsInRange(Pt.GetPosition(), Pt.Radius, 360.f, SoulBeacon->GetActorForwardVector(),
		                                    FMassEntityHandle(0, 0)));

	TArray<HashGridCell*> CaptureCellsCleaned = TArray<HashGridCell*>();
	TArray<HashGridCell*> EffectCellsCleaned = TArray<HashGridCell*>();

	for (const auto& Elem : CaptureCells)
		CaptureCellsCleaned.AddUnique(Elem);
	for (const auto& Elem : EffectCells)
		EffectCellsCleaned.AddUnique(Elem);

	SoulBeacon->AddCells(CaptureCells);
	for (auto& Cell : CaptureCellsCleaned)
		Cell->LinkedBeacon = SoulBeacon;

	for (auto& Cell : EffectCellsCleaned)
		Cell->LinkedBeacon = SoulBeacon;

	return true;
}

bool ASpatialHashGrid::SetBossAsKnown(ABoss* Boss)
{
	if (Instance->AllBosses.Contains(Boss)) return false;

	Instance->AllBosses.AddUnique(Boss);

	TArray<HashGridCell*> Cells = FindCellsInRange(Boss->GetActorLocation(), Boss->GetSummonRange(), 360.f,
	                                               Boss->GetActorForwardVector(), FMassEntityHandle(0, 0));
	for (auto& Cell : Cells)
	{
		Cell->LinkedBoss = Boss;
	}

	return true;
}

bool ASpatialHashGrid::RemoveBossAsKnown(ABoss* Boss)
{
	if (!Instance->AllBosses.Contains(Boss)) return false;

	Instance->AllBosses.Remove(Boss);

	TArray<HashGridCell*> Cells = FindCellsInRange(Boss->GetActorLocation(), Boss->GetSummonRange(), 360.f,
	                                               Boss->GetActorForwardVector(), FMassEntityHandle(0, 0));
	for (auto& Cell : Cells)
	{
		Cell->LinkedBoss = nullptr;
	}

	return true;
}

bool ASpatialHashGrid::Contains(FMassEntityHandle Entity)
{
	return Instance->PresentEntities.Contains(Entity);
}

bool ASpatialHashGrid::Contains(FVector Location, TWeakObjectPtr<ABuildingParent> Building)
{
	FIntVector2 GridCoords = WorldToGridCoords(Location);
	int Index = CoordsToIndex(GridCoords);
	return Instance->GridCells[Index].Contains(Building);
}

bool ASpatialHashGrid::Contains(FVector Location, TWeakObjectPtr<ALDElement> LDElem)
{
	FIntVector2 GridCoords = WorldToGridCoords(Location);
	int Index = CoordsToIndex(GridCoords);
	return Instance->GridCells[Index].Contains(LDElem);
}

FIntVector2 ASpatialHashGrid::CoordsFromHandle(FMassEntityHandle Entity)
{
	return Instance->HandleToCoordsMap[Entity];
}

int ASpatialHashGrid::CoordsToIndex(FIntVector2 Coords)
{
	return Coords.X + Coords.Y * Instance->GridSize.X;
}

void ASpatialHashGrid::RefreshPresent()
{
	Instance->HandleToCoordsMap.GenerateKeyArray(Instance->PresentEntities);
}

const GridCellEntityData ASpatialHashGrid::GetEntityData(FMassEntityHandle Entity)
{
	if (!Contains(Entity)) return GridCellEntityData::None();

	//checkf(Contains(Entity), TEXT("SpatialHashGrid Error : Accessing unknown Entity"));

	FIntVector2 EntityCoords = Instance->HandleToCoordsMap[Entity];
	//checkf(IsInGrid(EntityCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if (!IsInGrid(EntityCoords)) return GridCellEntityData::None();

	int CoordsIndex = CoordsToIndex(EntityCoords);

	GridCellEntityData* ToReturn = new GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f, 0.f,
	                                                      EEntityType::EntityTypeNone);

	if (Instance->GridCells[CoordsIndex].Contains(Entity))
	{
		ToReturn = Instance->GridCells[CoordsIndex].GetEntity(Entity);
	}

	return *ToReturn;
}

GridCellEntityData* ASpatialHashGrid::GetMutableEntityData(FMassEntityHandle Entity)
{
	if (!Instance->HandleToCoordsMap.Contains(Entity)) return nullptr;
	FIntVector2 EntityCoords = Instance->HandleToCoordsMap[Entity];
	//checkf(IsInGrid(EntityCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if (!DebugCheckExpr(IsInGrid(EntityCoords), "SpatialHashGrid Error : Accessing coords out of grid",
	                    Instance->bUseAsserts)) return nullptr;

	int CoordsIndex = CoordsToIndex(EntityCoords);

	GridCellEntityData* ToReturn = new GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f, 0.f,
	                                                      EEntityType::EntityTypeNone);

	if (Instance->GridCells[CoordsIndex].Contains(Entity))
	{
		ToReturn = Instance->GridCells[CoordsIndex].GetEntity(Entity);
	}

	return ToReturn;
}

bool ASpatialHashGrid::IsInGrid(FVector WorldCoordinates)
{
	FVector TLCellCorner = Instance->GridLocation;

	if (Instance->bIsPivotCentered)
	{
		//TLCellCorner.X -= Instance->CellSize.X * (GetGridSize().X / 2 + GetGridSize().Y % 2 * .5f);
		//TLCellCorner.Y -= Instance->CellSize.Y * (GetGridSize().Y / 2 + GetGridSize().Y % 2 * .5f);
	}

	FVector BRCellCorner = TLCellCorner + FVector(GetGridSize().X * Instance->CellSize.X,
	                                              GetGridSize().Y * Instance->CellSize.Y, .0f);;

	bool bIsOutGrid = (WorldCoordinates.X < TLCellCorner.X
		|| WorldCoordinates.X > BRCellCorner.X
		|| WorldCoordinates.Y < TLCellCorner.Y
		|| WorldCoordinates.Y > BRCellCorner.Y);

	return !bIsOutGrid;
}

bool ASpatialHashGrid::IsInGrid(FIntVector2 GridCoordinates)
{
	if (GridCoordinates.X < 0 || GridCoordinates.Y < 0 || GridCoordinates.X >= Instance->GridSize.X || GridCoordinates.Y
		>= Instance->GridSize.Y)
		return false;

	return true;
}

TArray<GridCellEntityData> ASpatialHashGrid::GetEntitiesInCell(FIntVector2 Coordinates)
{
	int CellIndex = CoordsToIndex(Coordinates);
	TArray<GridCellEntityData> Entities = Instance->GridCells[CellIndex].GetEntities();
	return Entities;
}

HashGridCell* ASpatialHashGrid::GetCellRef(FMassEntityHandle Handle)
{
	int32 Index = CoordsToIndex(CoordsFromHandle(Handle));
	return &Instance->GridCells[Index];
}

HashGridCell* ASpatialHashGrid::GetCellRef(FIntVector2 GridCoords)
{
	int32 Index = CoordsToIndex(GridCoords);
	return &Instance->GridCells[Index];
}

HashGridCell* ASpatialHashGrid::GetCellRef(FVector WorldCoords)
{
	int32 Index = CoordsToIndex(WorldToGridCoords(WorldCoords));
	return &Instance->GridCells[Index];
}

FIntVector2 ASpatialHashGrid::WorldToGridCoords(FVector WorldCoordinates)
{
	//checkf(IsInGrid(WorldCoordinates), TEXT("SpatialHashGrid : Entity Coordinates out of grid"));
	if (!DebugCheckExpr(IsInGrid(WorldCoordinates), "SpatialHashGrid Error : Entity Coordinates out of grid",
	                    Instance->bUseAsserts)) return FIntVector2(0, 0);

	FVector TopLeftCellCoordinates = Instance->GridLocation;

	if (Instance->bIsPivotCentered)
	{
		//TopLeftCellCoordinates.X -= Instance->CellSize.X * (GetGridSize().X / 2 + GetGridSize().X % 2 * .5f);
		//TopLeftCellCoordinates.Y -= Instance->CellSize.Y * (GetGridSize().Y / 2 + GetGridSize().Y % 2 * .5f);
	}

	FVector InGridCoordinates = WorldCoordinates - TopLeftCellCoordinates;

	int32 XCoordinates = FMath::FloorToInt(InGridCoordinates.X / Instance->CellSize.X);
	int32 YCoordinates = FMath::FloorToInt(InGridCoordinates.Y / Instance->CellSize.Y);

	return FIntVector2(XCoordinates, YCoordinates);
}

FIntVector2 ASpatialHashGrid::WorldToGridCoordsLocal(FVector WorldCoordinates)
{
	//checkf(IsInGrid(WorldCoordinates), TEXT("SpatialHashGrid : Entity Coordinates out of grid"));
	FVector BaseLocation = GetActorLocation();
	FVector InGridCoordinates = WorldCoordinates - BaseLocation;

	int32 XCoordinates = FMath::FloorToInt(InGridCoordinates.X / CellSizeX);
	int32 YCoordinates = FMath::FloorToInt(InGridCoordinates.Y / CellSizeY);

	return FIntVector2(XCoordinates, YCoordinates);
}

void ASpatialHashGrid::DrawDebugLineAt(FIntVector2 Start, FIntVector2 End, FColor Color)
{
	auto From = FVector(Start.X * CellSizeX, Start.Y * CellSizeY, 400.f);
	auto To = FVector(End.X * CellSizeX, End.Y * CellSizeY, 400.f);

	DrawDebugLine(Instance->GetWorld(), From, To, Color, false, DebugDuration);
}

void ASpatialHashGrid::DrawDebugLineAt(FVector2D Start, FVector2D End, FColor Color)
{
	auto From = FVector(Start.X * CellSizeX, Start.Y * CellSizeY, 400.f);
	auto To = FVector(End.X * CellSizeX, End.Y * CellSizeY, 400.f);

	DrawDebugLine(Instance->GetWorld(), From, To, Color, false, DebugDuration);
}

bool ASpatialHashGrid::IsValid()
{
	return !Instance->GridCells.IsEmpty();
}

int32 ASpatialHashGrid::GetEntitiesCount()
{
	return Instance->PresentEntities.Num();
}


TArray<FVector2D> ASpatialHashGrid::GetAllEntityOfTypeOfTeam(EEntityType Type, ETeam Team)
{
	TArray<FVector2D> Entities;
	FCriticalSection Mutex;
	Entities.Reserve(4500); // estimation

	ParallelFor(Instance->GridCells.Num(), [&](int32 Index)
	{
		const auto& Cell = Instance->GridCells[Index];
		TArray<FVector2D> LocalEntities;

		for (const auto& Pair : Cell.Entities)
		{
			if (const auto& EntityData = Pair.Value; EntityData.EntityType == Type && EntityData.Owner.Team == Team)
			{
				LocalEntities.Add(FVector2D(EntityData.Location.X, EntityData.Location.Y));
			}
		}

		if (LocalEntities.Num() > 0)
		{
			FScopeLock Lock(&Mutex);
			Entities.Append(LocalEntities);
		}
	});
	return Entities;
}

TArray<FMassEntityHandle> ASpatialHashGrid::GetAllEntityHandles()
{
	TArray<TArray<FMassEntityHandle>> ThreadLocalEntities;
	ThreadLocalEntities.SetNum(Instance->GridCells.Num());

	ParallelFor(Instance->GridCells.Num(), [&](int32 Index)
	{
		const auto& Cell = Instance->GridCells[Index];
		for (const auto& Pair : Cell.Entities)
		{
			ThreadLocalEntities[Index].Add(Pair.Key);
		}
	});

	TArray<FMassEntityHandle> Entities;
	Entities.Reserve(4500);
	for (const auto& LocalArray : ThreadLocalEntities)
	{
		Entities.Append(LocalArray);
	}
	return Entities;
}

FDetectionResult ASpatialHashGrid::FindClosestElementsInRange(FVector WorldCoordinates, float Range, float Angle,
                                                              FVector EntityForwardVector, FMassEntityHandle Entity)
{
	FDetectionResult Result;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectedEntities = 0;

	ETeam CallerTeam = GetEntityData(Entity).Owner.Team;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			TArray<FMassEntityHandle> Keys;
			Instance->GridCells[Index].Entities.GenerateKeyArray(Keys);

#if WITH_EDITOR
			int FoundCounter = 0;
			bool FullDetected = false;
#endif

			for (auto Key : Keys)
			{
				GridCellEntityData Data = Instance->GridCells[Index].Entities[Key];
				if (CallerTeam == Data.Owner.Team) continue;

#if WITH_EDITOR
				++FoundCounter;
#endif

				auto Direction = (Data.Location - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(
					FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				if (AngleBetween >= Angle) continue;

				auto Distance = (Data.Location - DetectionCenter).Length() - Data.TargetableRadius;
				if (Distance > Range || Distance > Result.EntityDistance) continue;

#if WITH_EDITOR
				FullDetected = true;
#endif

				Result.EntityDistance = Distance;
				Result.Entity = Key;

				/*FColor DebugColor = AngleBetween <= Angle ? FColor::Green : FColor::Red;
				DrawDebugLine(Instance->GetWorld(), DetectionCenter, Data.Location, DebugColor, false, 3.f, 0, 100);*/
			}

			for (auto Building : Instance->GridCells[Index].GetBuildings())
			{
				if (!Building.IsValid()) continue;
				if (Building->GetOwner().Team == CallerTeam) continue;

#if WITH_EDITOR
				++FoundCounter;
#endif

				auto Direction = (Building->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(
					FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector.GetSafeNormal())));
				if (AngleBetween >= Angle) continue;

				float Distance = (Building->GetActorLocation() - DetectionCenter).Length() - Building->
					GetTargetableRange();
				if (Distance > Range || Distance > Result.BuildingDistance) continue;

#if WITH_EDITOR
				FullDetected = true;
#endif

				Result.BuildingDistance = Distance;
				Result.Building = Building;
			}

			for (auto LD : Instance->GridCells[Index].GetLDElements())
			{
#if WITH_EDITOR
				++FoundCounter;
#endif

				float LDTargetableRange = 0.f;
				if (LD->GetLDElementType() == ELDElementType::LDElementNeutralCampType)
					LDTargetableRange = Cast<ANeutralCamp>(LD)->GetTargetableRange();

				auto Direction = (LD->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(
					FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector.GetSafeNormal())));
				if (AngleBetween >= Angle) continue;

				float Distance = (LD->GetActorLocation() - DetectionCenter).Length() - LDTargetableRange;
				if (Distance > Range || Distance > Result.LDDistance) continue;

#if WITH_EDITOR
				FullDetected = true;
#endif

				Result.LDDistance = Distance;
				Result.LD = LD;
			}

			//#if WITH_EDITOR
			//FColor Color = FullDetected ? FColor::Green : (FoundCounter != 0 ? FColor::Yellow : FColor::Red);
			//DebugSingleDetectionCell(WorldCoordinates, x, y, Color);
			//#endif // WITH_EDITOR
		}
	}

	return Result;
}

/*
* Returns an array of entites gathered from cells up to a Range distance from the center cell
* @param WorldCoordinates Position of the entity in the center cell
* @param Range Distance to the farthest cell in which to search
* 
* @return Returns the data linked to the found entities
*/
TMap<FMassEntityHandle, GridCellEntityData> ASpatialHashGrid::FindEntitiesInRange(
	FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity,
	ETeam Team)
{
	TMap<FMassEntityHandle, GridCellEntityData> FoundEntities = TMap<FMassEntityHandle, GridCellEntityData>();

	if (Instance->GameEnded) return FoundEntities;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectedEntities = 0;


	ETeam CallerTeam = !Entity.IsValid() ? Team : Contains(Entity) ? GetEntityData(Entity).Owner.Team : Team;

	//DrawDebugLine(Instance->GetWorld(), DetectionCenter, DetectionCenter + EntityForwardVector * 1000000, FColor::Purple, false, 10.f, 10U);

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			TArray<FMassEntityHandle> Keys;
			Instance->GridCells[Index].Entities.GenerateKeyArray(Keys);

			for (auto Key : Keys)
			{
				GridCellEntityData Data = Instance->GridCells[Index].Entities[Key];


				auto Distance = (Data.Location - DetectionCenter).Length() - Data.TargetableRadius;
				if (Distance > Range) continue;
				auto Direction = (Data.Location - DetectionCenter).GetSafeNormal();

				auto AngleBetween = FMath::RadiansToDegrees(
					FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));

				// DebugAmalgamDetection(DetectionCenter, Range, Angle, EntityForwardVector, AngleBetween <= Angle, Data.Location);

				if (AngleBetween <= Angle) FoundEntities.Add(Key, Data);
				if (CallerTeam != Data.Owner.Team)
				{
					FColor DebugColor = AngleBetween <= Angle ? FColor::Green : FColor::Red;
					//DrawDebugLine(Instance->GetWorld(), DetectionCenter, Data.Location, DebugColor, false, 3.f, 0, 100);
				}

				if (Key != Entity)
					++DetectedEntities;

				if (DetectedEntities >= Instance->MaxDetectableEntities)
				{
					FoundEntities.Remove(Entity);
					return FoundEntities;
				}
			}
		}
	}
	FoundEntities.Remove(Entity); // Prevents entity finding itself

	return FoundEntities;
}


TArray<TWeakObjectPtr<ABuildingParent>> ASpatialHashGrid::FindBuildingsInRange(
	FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<TWeakObjectPtr<ABuildingParent>> FoundBuildings;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectableEntities = 0;
	float DetectorTargetableRange = 62.5f;

	if (Instance->PresentEntities.Contains(Entity))
		DetectorTargetableRange = Instance->GetEntityData(Entity).TargetableRadius;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			for (auto Building : Instance->GridCells[Index].GetBuildings())
			{
				//DrawDebugSphere(Instance->GetWorld(), Building->GetActorLocation(), Building->GetTargetableRange(), 10, FColor::Orange, false, 2.f);
				if (!Building.IsValid()) continue;
				float Distance = (Building->GetActorLocation() - DetectionCenter).Length() - Building->
					GetTargetableRange() - DetectorTargetableRange;
				if (Distance > Range)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("Building out of range")));
					continue;
				}

				auto Direction = (Building->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(
					FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector.GetSafeNormal())));

				//DebugAmalgamDetection(DetectionCenter, Range, Angle, EntityForwardVector, AngleBetween <= Angle, Building->GetActorLocation());
				if (AngleBetween <= Angle && !FoundBuildings.Contains(Building)) FoundBuildings.Add(Building);

				FColor DebugColor = AngleBetween <= Angle ? FColor::Green : FColor::Red;

				//DrawDebugLine(Instance->GetWorld(), DetectionCenter, Building->GetActorLocation(), DebugColor, false, 2.f);

				++DetectableEntities;

				if (DetectableEntities >= Instance->MaxDetectableEntities)
					return FoundBuildings;
			}
		}
	}

	return FoundBuildings;
}

TArray<TWeakObjectPtr<ALDElement>> ASpatialHashGrid::FindLDElementsInRange(
	FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<TWeakObjectPtr<ALDElement>> FoundLD;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectableEntities = 0;
	float DetectorTargetableRange = 62.5f;

	if (Instance->PresentEntities.Contains(Entity))
		DetectorTargetableRange = Instance->GetEntityData(Entity).TargetableRadius;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			for (auto Elem : Instance->GridCells[Index].GetLDElements())
			{
				if (!Elem.IsValid()) continue;

				float Distance = (Elem->GetActorLocation() - DetectionCenter).Length() - DetectorTargetableRange;
				if (Distance > Range) continue;

				auto Direction = (Elem->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(
					FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				if (AngleBetween <= Angle) FoundLD.Add(Elem);

				//DebugAmalgamDetection(DetectionCenter, Range, Angle, EntityForwardVector, AngleBetween <= Angle, Elem->GetActorLocation());

				++DetectableEntities;
				if (DetectableEntities >= Instance->MaxDetectableEntities)
					return FoundLD;
			}
		}
	}

	return FoundLD;
}

TArray<HashGridCell*> ASpatialHashGrid::FindCellsInRange(FVector WorldCoordinates, float Range, float Angle,
                                                         FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<HashGridCell*> FoundCells;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectableEntities = 0;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			HashGridCell* ToAdd = &Instance->GridCells[Index];
			FoundCells.Add(ToAdd);
		}
	}

	return FoundCells;
}

TMap<FMassEntityHandle, GridCellEntityData> ASpatialHashGrid::FindEntitiesAroundCell(
	FVector WorldCoordinates, int32 Range)
{
	TMap<FMassEntityHandle, GridCellEntityData> FoundEntities;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	for (int x = -Range; x <= Range; ++x)
	{
		for (int y = -Range; y <= Range; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;


			FoundEntities.Append(Instance->GridCells[Index].Entities);
		}
	}

	return FoundEntities;
}

FMassEntityHandle ASpatialHashGrid::FindClosestEntity(FVector WorldCoordinates, float Range, float Angle,
                                                      FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team)
{
	TMap<FMassEntityHandle, GridCellEntityData> FoundEntities = Instance->FindEntitiesInRange(
		WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundEntities.Num() == 0) return FMassEntityHandle(0, 0); // Returns unset handle
	TArray<FMassEntityHandle> KeyArray;
	FoundEntities.GenerateKeyArray(KeyArray);


	FMassEntityHandle Closest = FMassEntityHandle(0, 0); //KeyArray[0];
	GridCellEntityData* ClosestData = nullptr; //&FoundEntities[Closest];
	float ClosestDistance = std::numeric_limits<float>::max(); //(ClosestData->Location - WorldCoordinates).Length();

	for (auto Key : KeyArray)
	{
		GridCellEntityData* Next = &FoundEntities[Key];
		float NextDistance = (Next->Location - WorldCoordinates).Length() - Next->TargetableRadius;

		//if (Next->AggroCount >= GetMaxEntityAggroCount()) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("FindClosestEntity \n\t Entity skipped > Max Aggro Reached"));

		if (NextDistance > ClosestDistance || Next->Owner.Team == Team || Next->AggroCount >= GetMaxEntityAggroCount())
			continue;

		Closest = Key;
		ClosestData = Next;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

TWeakObjectPtr<ABuildingParent> ASpatialHashGrid::FindClosestBuilding(FVector WorldCoordinates, float Range,
                                                                      float Angle, FVector EntityForwardVector,
                                                                      FMassEntityHandle Entity, ETeam Team)
{
	TArray<TWeakObjectPtr<ABuildingParent>> FoundBuildings = Instance->FindBuildingsInRange(
		WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundBuildings.Num() == 0) return nullptr;

	TWeakObjectPtr<ABuildingParent> Closest = nullptr;
	float ClosestDistance = std::numeric_limits<float>::max();

	for (auto Building : FoundBuildings)
	{
		float NextDistance = (Building->GetActorLocation() - WorldCoordinates).Length();
		if (NextDistance > ClosestDistance || Building->GetOwner().Team == Team) continue;

		Closest = Building;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

/*
* @todo : Remake evaluation after LDElements have teams (if they ever have)
*/
TWeakObjectPtr<ALDElement> ASpatialHashGrid::FindClosestLDElement(FVector WorldCoordinates, float Range, float Angle,
                                                                  FVector EntityForwardVector, FMassEntityHandle Entity,
                                                                  ETeam Team)
{
	TArray<TWeakObjectPtr<ALDElement>> FoundElems = Instance->FindLDElementsInRange(
		WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundElems.Num() == 0) return nullptr;

	float ClosestDistance = std::numeric_limits<float>::max();
	TWeakObjectPtr<ALDElement> Closest = nullptr;

	for (auto Elem : FoundElems)
	{
		if (!Elem.IsValid()) continue;
		if (!Elem->IsAttackableBy(Team)) continue;
		float NextDistance = (Elem->GetActorLocation() - WorldCoordinates).Length();
		if (NextDistance > ClosestDistance) continue;

		Closest = Elem;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

TWeakObjectPtr<ASoulBeacon> ASpatialHashGrid::IsInSoulBeaconRange(FMassEntityHandle Entity)
{
	if (!ASpatialHashGrid::Contains(Entity)) return nullptr;

	int Index = CoordsToIndex(CoordsFromHandle(Entity));
	FVector Location = Instance->GridCells[Index].Entities[Entity].Location;

	for (auto& CheckedBeacon : Instance->AllSoulBeacons)
	{
		if (!CheckedBeacon->IsValidRewarder()) continue;
		for (auto& Range : CheckedBeacon->GetEffectRanges())
		{
			//DebugDetectionRange(Range.Center, Range.Radius);
			float Distance = (Range.GetPosition() - Location).Length();
			if (Distance < Range.Radius)
			{
				return CheckedBeacon;
			}
		}
	}

	return nullptr;
}

TWeakObjectPtr<ASoulBeacon> ASpatialHashGrid::IsInSoulBeaconRangeByCell(HashGridCell* Cell)
{
	for (auto& CheckedBeacon : Instance->AllSoulBeacons)
	{
		if (CheckedBeacon->IsCellInRange(Cell)) return CheckedBeacon;
	}

	return nullptr;
}

TWeakObjectPtr<ASoulBeacon> ASpatialHashGrid::IsInSoulBeaconRange(FVector WorldCoordinates)
{
	for (auto& CheckedBeacon : Instance->AllSoulBeacons)
		for (auto& Range : CheckedBeacon->GetEffectRanges())
		{
			float Distance = (Range.GetPosition() - WorldCoordinates).Length();
			if (Distance < Range.Radius)
			{
				return CheckedBeacon;
			}
		}

	return nullptr;
}

TWeakObjectPtr<ABoss> ASpatialHashGrid::IsInBossRange(FMassEntityHandle Entity)
{
	if (!ASpatialHashGrid::Contains(Entity)) return nullptr;

	int Index = CoordsToIndex(CoordsFromHandle(Entity));
	FVector Location = Instance->GridCells[Index].Entities[Entity].Location;

	for (auto& CheckedBoss : Instance->AllBosses)
	{
		//DebugDetectionRange(Range.Center, Range.Radius);
		float Distance = (CheckedBoss->GetActorLocation() - Location).Length();
		if (Distance < CheckedBoss->GetSummonRange())
		{
			return CheckedBoss;
		}
	}

	return nullptr;
}

TWeakObjectPtr<ABoss> ASpatialHashGrid::IsInBossRange(FVector WorldCoordinates)
{
	FVector Location = WorldCoordinates;

	for (auto& CheckedBoss : Instance->AllBosses)
	{
		//DebugDetectionRange(Range.Center, Range.Radius);
		float Distance = (CheckedBoss->GetActorLocation() - Location).Length();
		if (Distance < CheckedBoss->GetSummonRange())
		{
			return CheckedBoss;
		}
	}

	return nullptr;
}

bool ASpatialHashGrid::IsInBossRange(ABoss* Boss, FMassEntityHandle Entity)
{
	if (!Instance->AllBosses.Contains(Boss)) return false;
	if (!ASpatialHashGrid::Contains(Entity)) return false;

	int Index = CoordsToIndex(CoordsFromHandle(Entity));
	FVector Location = Instance->GridCells[Index].Entities[Entity].Location;

	float Distance = (Boss->GetActorLocation() - Location).Length();
	return Distance < Boss->GetSummonRange();
}

void ASpatialHashGrid::DebugAmalgamDetection(FVector WorldCoordinates, float Range, float Angle, FVector ForwardVector,
                                             bool Detected, FVector TargetLocation)
{
	if (Instance->bDebugAmalgamDetectionCone) DebugDetectionCone(WorldCoordinates, Range, Angle, ForwardVector,
	                                                             Detected);
	if (Instance->bDebugAmalgamDetectionCheck) DebugDetectionCheck(WorldCoordinates, TargetLocation, Range, Angle,
	                                                               ForwardVector, Detected);
}

void ASpatialHashGrid::DebugDetectionRange(FVector WorldCoordinates, float Range)
{
	DrawDebugSphere(Instance->GetWorld(), WorldCoordinates, Range, 10, FColor::Orange, false, 10.f);
}

void ASpatialHashGrid::DebugDetectionCheck(FVector WorldCoordinates, FVector TargetPosition, float Range, float Angle,
                                           FVector ForwardVector, bool DetectionCheckValue)
{
	FColor DebugColor;
	if (DetectionCheckValue)
		DebugColor = FColor::Green;
	else
		DebugColor = FColor::Red;

	DrawDebugLine(Instance->GetWorld(), WorldCoordinates, TargetPosition, DebugColor, false, 10.f);
}

void ASpatialHashGrid::DebugDetectionCone(FVector WorldCoordinates, float Range, float Angle, FVector Forward,
                                          bool Detected)
{
	FColor Color = Detected ? FColor::Green : FColor::Red;
	DrawDebugCone(Instance->GetWorld(), WorldCoordinates, Forward, Range, FMath::DegreesToRadians(Angle),
	              FMath::DegreesToRadians(Angle), 10, Color, false, 1.f);
}

void ASpatialHashGrid::DebugDetectionCell(FVector WorldCoordinates, float Range)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			float GridXPos = Instance->GetActorLocation().X + (GridCoords.X + x) * Instance->CellSize.X;
			float GridYPos = Instance->GetActorLocation().Y + (GridCoords.Y + y) * Instance->CellSize.Y;

			FVector TL = FVector(GridXPos, GridYPos, 0.f);
			FVector TR = FVector(GridXPos + Instance->CellSize.X, GridYPos, 0.f);
			FVector BL = FVector(GridXPos, GridYPos + Instance->CellSize.Y, 0.f);
			FVector BR = FVector(GridXPos + Instance->CellSize.X, GridYPos + Instance->CellSize.Y, 0.f);

			DrawDebugLine(Instance->GetWorld(), TL, TR, FColor::Orange, false, 1.0f);
			DrawDebugLine(Instance->GetWorld(), TL, BL, FColor::Orange, false, 1.0f);
			DrawDebugLine(Instance->GetWorld(), BL, BR, FColor::Orange, false, 1.0f);
			DrawDebugLine(Instance->GetWorld(), TR, BR, FColor::Orange, false, 1.0f);
		}
	}
}

void ASpatialHashGrid::DebugSingleDetectionCell(FVector WorldCoordinates, int XOffset, int YOffset, FColor CellColor)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	float GridXPos = Instance->GetActorLocation().X + (GridCoords.X + XOffset) * Instance->CellSize.X;
	float GridYPos = Instance->GetActorLocation().Y + (GridCoords.Y + YOffset) * Instance->CellSize.Y;

	FVector TL = FVector(GridXPos, GridYPos, 0.f);
	FVector TR = FVector(GridXPos + Instance->CellSize.X, GridYPos, 0.f);
	FVector BL = FVector(GridXPos, GridYPos + Instance->CellSize.Y, 0.f);
	FVector BR = FVector(GridXPos + Instance->CellSize.X, GridYPos + Instance->CellSize.Y, 0.f);

	DrawDebugLine(Instance->GetWorld(), TL, TR, CellColor, false, 2.5f, 10U);
	DrawDebugLine(Instance->GetWorld(), TL, BL, CellColor, false, 2.5f, 10U);
	DrawDebugLine(Instance->GetWorld(), BL, BR, CellColor, false, 2.5f, 10U);
	DrawDebugLine(Instance->GetWorld(), TR, BR, CellColor, false, 2.5f, 10U);
}


bool ASpatialHashGrid::DebugCheckExpr(bool Expr, const char* Msg, bool bUseAsserts)
{
	if (Instance->bUseAsserts)
	{
		checkf(Expr, TEXT("checkf triggered."));
		return true;
	}

	return Expr;
}

void ASpatialHashGrid::DebugAllBuildingRanges()
{
	for (auto& Cell : Instance->GridCells)
	{
		for (auto& Building : Cell.Buildings)
		{
			DrawDebugSphere(Instance->GetWorld(), Building->GetActorLocation(), Building->GetTargetableRange(), 10,
			                FColor::Orange, false, 10.f, 10U);
		}
	}
}

void ASpatialHashGrid::DebugAllLDRanges()
{
	for (auto& Cell : Instance->GridCells)
	{
		for (auto& Elem : Cell.LDElements)
		{
			DrawDebugSphere(Instance->GetWorld(), Elem->GetActorLocation(), 1500.f /*Placeholder value*/, 10,
			                FColor::Orange, false, 10.f, 10U);
		}
	}
}

void ASpatialHashGrid::DebugGridContent()
{
	int LDCount = 0;
	int BuildingCount = 0;
	for (auto& Cell : Instance->GridCells)
	{
		LDCount += Cell.GetLDElementsNum();
		BuildingCount += Cell.GetBuildingsNum();
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan,
	                                 FString::Printf(TEXT("%d LD Elements \n %d Buildings"), LDCount, BuildingCount));
}

void ASpatialHashGrid::DebugBossRange()
{
	for (const auto& Boss : Instance->AllBosses)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange,
		                                 FString::Printf(TEXT("Boss Radius : %f"), Boss->GetSummonRange()));
		DrawDebugSphere(Instance->GetWorld(), Boss->GetActorLocation(), Boss->GetSummonRange(), 8, FColor::Orange,
		                false, 5.f, 100U);
	}
}

// Called when the game starts or when spawned
void ASpatialHashGrid::BeginPlay()
{
	Super::BeginPlay();
	//Instance->GridCells.Empty();
	//Generate();
	Instance->GridLocation = GetActorLocation();
	InitGrid();
}

void ASpatialHashGrid::InitGrid()
{
	Instance->GridLocation = Instance->GetActorLocation();
	CellSize = FIntVector2(CellSizeX, CellSizeY);
	GridSize = FIntVector2(GridSizeX, GridSizeY);
	UE_LOG(LogTemp, Warning, TEXT("Grid Size : %d ; %d"), GridSizeX, GridSizeY);
	UE_LOG(LogTemp, Warning, TEXT("Cell Size : %f ; %f"), CellSizeX, CellSizeY);
	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Grid Size : %d ; %d"), GridSize.X, GridSize.Y));
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("yellow hello"));

	Generate();
	CalculateCollisionsLocal();
}

void ASpatialHashGrid::DebugGridSize()
{
	UE_LOG(LogTemp, Warning, TEXT("Grid Size : %d ; %d"), GridSizeX, GridSizeY);
	UE_LOG(LogTemp, Warning, TEXT("Cell Size : %f ; %f"), CellSizeX, CellSizeY);
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow,
	                                 FString::Printf(TEXT("Grid Size : %d ; %d"), GridSizeX, GridSizeY));
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow,
	                                 FString::Printf(TEXT("Cell Grid Size : %d ; %d"), CellSizeX, CellSizeY));
}

void ASpatialHashGrid::DebugGridVisuals()
{
	//Debug draw line
	FVector GridBase = GetActorLocation();
	if (bIsPivotCentered) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow,
	                                                       FString::Printf(
		                                                       TEXT("Grid is Centered, will be debuged latter")));

	for (int i = 0; i < GridSizeY; ++i)
	{
		FVector Start = GridBase + FVector(0, i * CellSizeY, 50);
		FVector End = Start + FVector(GridSizeX * CellSizeX, 0, 50);
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 10.0f, 0, 100.f);
	}
	for (int i = 0; i < GridSizeX; ++i)
	{
		FVector Start = GridBase + FVector(i * CellSizeX, 0, 50);
		FVector End = Start + FVector(0, GridSizeY * CellSizeY, 50);
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 10.0f, 0, 100.f);
	}
}

void ASpatialHashGrid::DebugGridValidity()
{
	FVector GridBase = GetActorLocation();
	//GenerateLocalGrid();

	GridCells = TArray<HashGridCell>();

	for (int32 Index = 0; Index < GridSizeX * GridSizeY; ++Index)
	{
		auto NewCell = HashGridCell();
		GridCells.Add(NewCell);
	}

	if (GridCells.Num() <= 0) return;

	/* Test data */
	// GridCells[0 + 0 * GridSizeX].CellIsValidForUnits = false;
	// GridCells[3 + 1 * GridSizeX].CellIsValidForUnits = false;
	// GridCells[3 + 3 * GridSizeX].CellIsValidForUnits = false;
	// GridCells[5 + 2 * GridSizeX].CellIsValidForUnits = false;

	CalculateCollisionsLocal();

	for (int i = 0; i < GridSizeX; ++i)
	{
		for (int j = 0; j < GridSizeY; ++j)
		{
			FVector CenterPos = GridBase + FVector((i + 0.5) * CellSizeX, (j + 0.5) * CellSizeY, 50);
			const auto Cell = GridCells[i + j * GridSizeX];
			auto Color = Cell.CellIsValidForUnits ? FColor::Green : FColor::Red;
			DrawDebugLine(GetWorld(), CenterPos, CenterPos + FVector(0, 0, 600), Color, false, 10.0f, 0, 100.f);
		}
	}
}

void ASpatialHashGrid::DebugGridValidityNoGenerate()
{
	FVector GridBase = GetActorLocation();
	GridCells = TArray<HashGridCell>();
	for (int32 Index = 0; Index < GridSizeX * GridSizeY; ++Index)
	{
		auto NewCell = HashGridCell();
		GridCells.Add(NewCell);
	}

	if (GridCells.Num() <= 0) return;

	CalculateCollisionsLocal();

	for (int i = 0; i < GridSizeX; ++i)
	{
		for (int j = 0; j < GridSizeY; ++j)
		{
			FVector CenterPos = GridBase + FVector((i + 0.5) * CellSizeX, (j + 0.5) * CellSizeY, 50);
			const auto Cell = GridCells[i + j * GridSizeX];
			auto Color = Cell.CellIsValidForUnits ? FColor::Green : FColor::Red;
			DrawDebugLine(GetWorld(), CenterPos, CenterPos + FVector(0, 0, 600), Color, false, 10.0f, 0, 100.f);
		}
	}
}

/*
* Differentiation between GenerateFromCenter & Generate is only useful if grid has a Gizmo
*/
bool ASpatialHashGrid::Generate()
{
	if (Instance->bIsPivotCentered)
		return GenerateGridFromCenter();

	return GenerateGrid();
}

bool ASpatialHashGrid::GenerateGrid()
{
	Instance->GridCells = TArray<HashGridCell>();

	for (int32 Index = 0; Index < Instance->GridSize.X * Instance->GridSize.Y; ++Index)
	{
		Instance->GridCells.Add(HashGridCell());
	}

	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("SHG / Generate Grid : %d cells in grid"), Instance->GridCells.Num()));
	return true;
}

void ASpatialHashGrid::GenerateLocalGrid()
{
	GridCells = TArray<HashGridCell>();

	for (int32 Index = 0; Index < GridSize.X * GridSize.Y; ++Index)
	{
		auto NewCell = HashGridCell();
		GridCells.Add(NewCell);
	}
}

void ASpatialHashGrid::CalculateCollisionsLocal()
{
	if (GridCells.Num() <= 0) return;

	/* Rectangle shapes */
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("FallRectange"), Actors);
	for (auto& Actor : Actors)
	{
		if (!Actor) continue;

		FVector Center = Actor->GetActorLocation();
		FVector Scale = Actor->GetActorScale();
		auto ActorRotation = Actor->GetActorRotation();

		FVector TL = FVector(-Scale.X, -Scale.Y, 0) * 50.0f;
		TL = ActorRotation.RotateVector(TL) + Center;
		FVector TR = FVector(Scale.X, -Scale.Y, 0) * 50.0f;
		TR = ActorRotation.RotateVector(TR) + Center;
		FVector BL = FVector(-Scale.X, Scale.Y, 0) * 50.0f;
		BL = ActorRotation.RotateVector(BL) + Center;
		FVector BR = FVector(Scale.X, Scale.Y, 0) * 50.0f;
		BR = ActorRotation.RotateVector(BR) + Center;

		auto TL2DInt = WorldToGridCoordsLocal(TL);
		auto TR2DInt = WorldToGridCoordsLocal(TR);
		auto BL2DInt = WorldToGridCoordsLocal(BL);
		auto BR2DInt = WorldToGridCoordsLocal(BR);
		auto Center2DInt = WorldToGridCoordsLocal(Center);

		auto TL2D = FVector2D(TL2DInt.X, TL2DInt.Y);
		auto TR2D = FVector2D(TR2DInt.X, TR2DInt.Y);
		auto BL2D = FVector2D(BL2DInt.X, BL2DInt.Y);
		auto BR2D = FVector2D(BR2DInt.X, BR2DInt.Y);
		auto Center2D = FVector2D(Center2DInt.X, Center2DInt.Y);

		/* Invalid cells in between */
		if (TL2D.X + TL2D.Y * GridSizeX < GridCells.Num() && TL2D.X + TL2D.Y * GridSizeX >= 0) GridCells[TL2D.X + TL2D.Y
			* GridSizeX].CellIsValidForUnits = false;
		if (TR2D.X + TR2D.Y * GridSizeX < GridCells.Num() && TR2D.X + TR2D.Y * GridSizeX >= 0) GridCells[TR2D.X + TR2D.Y
			* GridSizeX].CellIsValidForUnits = false;
		if (BL2D.X + BL2D.Y * GridSizeX < GridCells.Num() && BL2D.X + BL2D.Y * GridSizeX >= 0) GridCells[BL2D.X + BL2D.Y
			* GridSizeX].CellIsValidForUnits = false;
		if (BR2D.X + BR2D.Y * GridSizeX < GridCells.Num() && BR2D.X + BR2D.Y * GridSizeX >= 0) GridCells[BR2D.X + BR2D.Y
			* GridSizeX].CellIsValidForUnits = false;


		/* Rotate corners to default */
		auto ActorRotationToDefault = ActorRotation * -1;
		auto TLPrime = ActorRotationToDefault.RotateVector(TL - Center) + Center;
		auto TRPrime = ActorRotationToDefault.RotateVector(TR - Center) + Center;
		auto BLPrime = ActorRotationToDefault.RotateVector(BL - Center) + Center;
		auto BRPrime = ActorRotationToDefault.RotateVector(BR - Center) + Center;

		auto TL2DPrime = WorldToGridCoordsLocal(TLPrime);
		auto TR2DPrime = WorldToGridCoordsLocal(TRPrime);
		auto BL2DPrime = WorldToGridCoordsLocal(BLPrime);
		auto BR2DPrime = WorldToGridCoordsLocal(BRPrime);

		TArray<FVector2D> GridCellsToRotate = TArray<FVector2D>();
		auto Xmin = FMath::Min(
			FMath::Min(TL2DPrime.X, TR2DPrime.X),
			FMath::Min(BL2DPrime.X, BR2DPrime.X));
		auto Xmax = FMath::Max(
			FMath::Max(TL2DPrime.X, TR2DPrime.X),
			FMath::Max(BL2DPrime.X, BR2DPrime.X));
		auto Ymin = FMath::Min(
			FMath::Min(TL2DPrime.Y, TR2DPrime.Y),
			FMath::Min(BL2DPrime.Y, BR2DPrime.Y));
		auto Ymax = FMath::Max(
			FMath::Max(TL2DPrime.Y, TR2DPrime.Y),
			FMath::Max(BL2DPrime.Y, BR2DPrime.Y));


		for (int x = Xmin; x <= Xmax; ++x)
		{
			for (int y = Ymin; y <= Ymax; ++y)
			{
				GridCellsToRotate.Add(FVector2D(x, y));
			}
		}

		for (const auto& Cell : GridCellsToRotate)
		{
			const auto CellWorldPos = FVector(Cell.X * CellSizeX, Cell.Y * CellSizeY, 0) + GetActorLocation();
			auto CellWorldPosRotated = ActorRotation.RotateVector(CellWorldPos - Center) + Center;
			auto CellWorldPosRotated2D = WorldToGridCoordsLocal(CellWorldPosRotated);
			SetPosToFalseForRectangle(CellWorldPosRotated2D.X, CellWorldPosRotated2D.Y);
		}
	}


	/* Cleanup */
	for (int x = 1; x < GridSizeX - 1; ++x)
	{
		for (int y = 1; y < GridSizeY - 1; ++y)
		{
			int InvalidArround = 0;
			if (GridCells[x + 1 + y * GridSizeX].CellIsValidForUnits == false) InvalidArround++;
			if (GridCells[x - 1 + y * GridSizeX].CellIsValidForUnits == false) InvalidArround++;
			if (GridCells[x + (y + 1) * GridSizeX].CellIsValidForUnits == false) InvalidArround++;
			if (GridCells[x + (y - 1) * GridSizeX].CellIsValidForUnits == false) InvalidArround++;
			if (InvalidArround >= 3) GridCells[x + y * GridSizeX].CellIsValidForUnits = false;
		}
	}
}

void ASpatialHashGrid::SetPosToFalseForRectangle(int X, int Y)
{
	if (X + Y * GridSizeX >= GridCells.Num() || X + Y * GridSizeX < 0) return;;
	GridCells[X + Y * GridSizeX].CellIsValidForUnits = false;
}

FVector2D ASpatialHashGrid::GetLineEquation(FVector2D PointA, FVector2D PointB)
{
	return FVector2D();
}

bool ASpatialHashGrid::GenerateGridFromCenter()
{
	Instance->GridCells = TArray<HashGridCell>();

	for (int32 Index = 0; Index < Instance->GridSize.X * Instance->GridSize.Y; ++Index)
	{
		Instance->GridCells.Add(HashGridCell());
	}

	return true;
}

// Called every frame
void ASpatialHashGrid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void GridCellEntityData::DamageEntity(float Damage)
{
	EntityHealth -= Damage;
	//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Damage inflicted : %f \n\t New Health : %f"), Damage, EntityHealth));
}

GridCellEntityData* HashGridCell::GetEntity(FMassEntityHandle Entity)
{
	return &Entities[Entity];
}

const TArray<GridCellEntityData> HashGridCell::GetEntities()
{
	TArray<GridCellEntityData> ValueArray;
	Entities.GenerateValueArray(ValueArray);
	const TArray<GridCellEntityData> ConstCopy = ValueArray;

	return ConstCopy;
}

TArray<GridCellEntityData> HashGridCell::GetMutableEntities()
{
	TArray<GridCellEntityData> ValueArray;
	Entities.GenerateValueArray(ValueArray);
	return ValueArray;
}

TArray<TWeakObjectPtr<ABuildingParent>> HashGridCell::GetBuildings()
{
	TArray<TWeakObjectPtr<ABuildingParent>> OutBuildings = Buildings;
	return OutBuildings;
}

TArray<TWeakObjectPtr<ALDElement>> HashGridCell::GetLDElements()
{
	TArray<TWeakObjectPtr<ALDElement>> OutLDElems = LDElements;
	return OutLDElems;
}

TArray<FOwner> HashGridCell::GetPresentOwners()
{
	return PresentOwners;
}

void HashGridCell::UpdatePresentOwners()
{
	PresentOwners.Empty();

	for (auto& Entity : GetEntities())
	{
		if (!PresentOwners.Contains(Entity.Owner))
		{
			PresentOwners.AddUnique(Entity.Owner);
		}
	}

	if (LinkedBeacon.IsValid())
		LinkedBeacon->CheckCells();
}

bool HashGridCell::Contains(FMassEntityHandle Entity)
{
	return Entities.Contains(Entity);
}

bool HashGridCell::Contains(TWeakObjectPtr<ABuildingParent> Building)
{
	return Buildings.Contains(Building);
}

bool HashGridCell::Contains(TWeakObjectPtr<ALDElement> LDElement)
{
	return LDElements.Contains(LDElement);
}

int HashGridCell::GetTotalNumByTeam(FOwner Owner)
{
	ETeam Team = Owner.Team;

	TArray<GridCellEntityData> EntityValues;
	Entities.GenerateValueArray(EntityValues);

	int NumEntities = EntityValues.FilterByPredicate([&Team](GridCellEntityData EntityData)
	{
		return EntityData.Owner.Team == Team;
	}).Num();

	int NumBuildings = Buildings.FilterByPredicate([&Team](TWeakObjectPtr<ABuildingParent> Building)
	{
		return Building->GetOwner().Team == Team;
	}).Num();

	return NumEntities + NumBuildings;
}

int HashGridCell::GetTotalNumByTeamDifference(FOwner Owner)
{
	return GetTotalNum() - GetTotalNumByTeam(Owner);
}
